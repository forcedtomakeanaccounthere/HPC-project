const express = require('express');
const cors = require('cors');
const multer = require('multer');
const path = require('path');
const fs = require('fs');
const { exec } = require('child_process');
const mongoose = require('mongoose');
require('dotenv').config();

const app = express();
const PORT = process.env.PORT || 5000;

// MongoDB Connection (Optional - app works without it)
mongoose.connect(process.env.MONGODB_URI || 'mongodb://localhost:27017/image_processor', {
    useNewUrlParser: true,
    useUnifiedTopology: true
})
.then(() => console.log('✓ MongoDB connected - history feature enabled'))
.catch(err => {
    console.warn('⚠ MongoDB not connected - history feature disabled');
    console.warn('  To enable history, start MongoDB or update MONGODB_URI in .env');
});

// Image Processing Schema
const ImageSchema = new mongoose.Schema({
    originalName: String,
    uploadedPath: String,
    processedPath: String,
    filter: String,
    parameters: Object,
    processingTime: Number,
    createdAt: { type: Date, default: Date.now },
    userId: String
});

const ImageModel = mongoose.model('Image', ImageSchema);

// Middleware
app.use(cors());
app.use(express.json());
app.use('/uploads', express.static(path.join(__dirname, 'uploads')));
app.use('/processed', express.static(path.join(__dirname, 'processed')));

// Ensure directories exist
const ensureDirectories = () => {
    ['uploads', 'processed', 'public'].forEach(dir => {
        const dirPath = path.join(__dirname, dir);
        if (!fs.existsSync(dirPath)) {
            fs.mkdirSync(dirPath, { recursive: true });
        }
    });
};
ensureDirectories();

// Configure Multer for file uploads
const storage = multer.diskStorage({
    destination: (req, file, cb) => {
        cb(null, path.join(__dirname, 'uploads'));
    },
    filename: (req, file, cb) => {
        const uniqueName = `${Date.now()}-${Math.random().toString(36).substr(2, 9)}${path.extname(file.originalname)}`;
        cb(null, uniqueName);
    }
});

const upload = multer({
    storage: storage,
    limits: { fileSize: 10 * 1024 * 1024 }, // 10MB limit
    fileFilter: (req, file, cb) => {
        const allowedTypes = /jpeg|jpg|png|gif|bmp/;
        const extname = allowedTypes.test(path.extname(file.originalname).toLowerCase());
        const mimetype = allowedTypes.test(file.mimetype);
        
        if (extname && mimetype) {
            cb(null, true);
        } else {
            cb(new Error('Only image files are allowed!'));
        }
    }
});

// Helper function to run C processor
const runImageProcessor = (inputPath, outputPath, filter, params) => {
    return new Promise((resolve, reject) => {
        // Use local copy of executable with DLL dependencies
        const processorPath = path.join(__dirname, 'image_processor_api.exe');
        
        // Check if executable exists
        if (!fs.existsSync(processorPath)) {
            reject(new Error(`Executable not found at ${processorPath}. Copy image_processor_api.exe and required DLLs to backend directory.`));
            return;
        }
        
        // Build arguments array for safer execution
        const args = [inputPath, outputPath, filter];
        
        // Add parameters based on filter type
        if (params) {
            if (filter === 'blur' && params.sigma) {
                args.push(params.sigma.toString());
            } else if (filter === 'sharpen' && params.intensity) {
                args.push(params.intensity.toString());
            } else if (filter === 'noise' && params.level) {
                args.push(params.level.toString());
            } else if (filter === 'compress' && params.levels) {
                args.push(params.levels.toString());
            } else if (filter === 'brightness' && params.value !== undefined) {
                args.push(params.value.toString());
            } else if (filter === 'saturation' && params.value !== undefined) {
                args.push(params.value.toString());
            } else if (filter === 'rotate90' && params.times !== undefined) {
                args.push(params.times.toString());
            } else if (filter === 'rotate' && params.angle !== undefined) {
                args.push(params.angle.toString());
            }
        }
        
        console.log('Executing:', processorPath, args);
        
        const startTime = Date.now();
        const { execFile } = require('child_process');
        execFile(processorPath, args, { cwd: __dirname }, (error, stdout, stderr) => {
            const processingTime = Date.now() - startTime;
            
            if (error) {
                console.error('Execution error:', error);
                console.error('stderr:', stderr);
                console.error('stdout:', stdout);
                reject(error);
                return;
            }
            
            console.log('stdout:', stdout);
            resolve({ processingTime, output: stdout });
        });
    });
};

// ==================== API ROUTES ====================

// Health check
app.get('/api/health', (req, res) => {
    res.json({ status: 'OK', message: 'HPC Image Processor API is running' });
});

// Upload image
app.post('/api/upload', upload.single('image'), async (req, res) => {
    try {
        if (!req.file) {
            return res.status(400).json({ error: 'No file uploaded' });
        }
        
        let imageId = null;
        
        // Save to DB if MongoDB is connected
        if (mongoose.connection.readyState === 1) {
            const imageDoc = new ImageModel({
                originalName: req.file.originalname,
                uploadedPath: req.file.path,
                userId: req.body.userId || 'anonymous'
            });
            await imageDoc.save();
            imageId = imageDoc._id;
        }
        
        res.json({
            success: true,
            message: 'Image uploaded successfully',
            image: {
                id: imageId,
                filename: req.file.filename,
                originalName: req.file.originalname,
                path: `/uploads/${req.file.filename}`,
                size: req.file.size
            }
        });
    } catch (error) {
        console.error('Upload error:', error);
        res.status(500).json({ error: 'Failed to upload image' });
    }
});

// Process image
app.post('/api/process', async (req, res) => {
    try {
        const { filename, filter, parameters } = req.body;
        
        if (!filename || !filter) {
            return res.status(400).json({ error: 'Missing filename or filter' });
        }
        
        const inputPath = path.join(__dirname, 'uploads', filename);
        const outputFilename = `processed-${Date.now()}-${filename}`;
        const outputPath = path.join(__dirname, 'processed', outputFilename);
        
        if (!fs.existsSync(inputPath)) {
            return res.status(404).json({ error: 'Input file not found' });
        }
        
        // Run the image processor
        const result = await runImageProcessor(inputPath, outputPath, filter, parameters);
        
        // Update database if MongoDB is connected
        if (mongoose.connection.readyState === 1) {
            const imageDoc = await ImageModel.findOne({ uploadedPath: inputPath });
            if (imageDoc) {
                imageDoc.processedPath = outputPath;
                imageDoc.filter = filter;
                imageDoc.parameters = parameters;
                imageDoc.processingTime = result.processingTime;
                await imageDoc.save();
            }
        }
        
        res.json({
            success: true,
            message: 'Image processed successfully',
            result: {
                processedPath: `/processed/${outputFilename}`,
                processingTime: result.processingTime,
                filter: filter,
                parameters: parameters
            }
        });
        
    } catch (error) {
        console.error('Processing error:', error);
        res.status(500).json({ error: 'Failed to process image', details: error.message });
    }
});

// Batch process - apply multiple filters
app.post('/api/process-batch', async (req, res) => {
    try {
        const { filename, filters } = req.body;
        
        if (!filename || !filters || !Array.isArray(filters)) {
            return res.status(400).json({ error: 'Missing filename or filters array' });
        }
        
        let currentInput = path.join(__dirname, 'uploads', filename);
        let currentFilename = filename;
        const results = [];
        
        for (let i = 0; i < filters.length; i++) {
            const { filter, parameters } = filters[i];
            const outputFilename = `batch-${Date.now()}-step${i}-${currentFilename}`;
            const outputPath = path.join(__dirname, 'processed', outputFilename);
            
            const result = await runImageProcessor(currentInput, outputPath, filter, parameters);
            
            results.push({
                step: i + 1,
                filter: filter,
                parameters: parameters,
                processingTime: result.processingTime,
                path: `/processed/${outputFilename}`
            });
            
            // Use output as input for next filter
            currentInput = outputPath;
            currentFilename = outputFilename;
        }
        
        res.json({
            success: true,
            message: 'Batch processing completed',
            results: results,
            finalOutput: results[results.length - 1].path,
            totalTime: results.reduce((sum, r) => sum + r.processingTime, 0)
        });
        
    } catch (error) {
        console.error('Batch processing error:', error);
        res.status(500).json({ error: 'Failed to process batch', details: error.message });
    }
});

// Get processing history
app.get('/api/history', async (req, res) => {
    try {
        // Return empty if MongoDB not connected
        if (mongoose.connection.readyState !== 1) {
            return res.json({ success: true, history: [], message: 'MongoDB not connected' });
        }
        
        const { userId, limit = 20 } = req.query;
        
        const query = userId ? { userId } : {};
        const history = await ImageModel.find(query)
            .sort({ createdAt: -1 })
            .limit(parseInt(limit));
        
        res.json({
            success: true,
            history: history.map(img => ({
                id: img._id,
                originalName: img.originalName,
                filter: img.filter,
                parameters: img.parameters,
                processingTime: img.processingTime,
                createdAt: img.createdAt,
                uploadedPath: img.uploadedPath ? `/uploads/${path.basename(img.uploadedPath)}` : null,
                processedPath: img.processedPath ? `/processed/${path.basename(img.processedPath)}` : null
            }))
        });
    } catch (error) {
        console.error('History error:', error);
        res.status(500).json({ error: 'Failed to fetch history' });
    }
});

// Delete image
app.delete('/api/image/:id', async (req, res) => {
    try {
        const imageDoc = await ImageModel.findById(req.params.id);
        
        if (!imageDoc) {
            return res.status(404).json({ error: 'Image not found' });
        }
        
        // Delete files
        if (imageDoc.uploadedPath && fs.existsSync(imageDoc.uploadedPath)) {
            fs.unlinkSync(imageDoc.uploadedPath);
        }
        if (imageDoc.processedPath && fs.existsSync(imageDoc.processedPath)) {
            fs.unlinkSync(imageDoc.processedPath);
        }
        
        await ImageModel.findByIdAndDelete(req.params.id);
        
        res.json({ success: true, message: 'Image deleted successfully' });
    } catch (error) {
        console.error('Delete error:', error);
        res.status(500).json({ error: 'Failed to delete image' });
    }
});

// Get available filters
app.get('/api/filters', (req, res) => {
    res.json({
        success: true,
        filters: [
            {
                id: 'grayscale',
                name: 'Black & White',
                description: 'Convert image to grayscale',
                hasSlider: false,
                icon: '⚫'
            },
            {
                id: 'blur',
                name: 'Blur',
                description: 'Apply Gaussian blur',
                hasSlider: true,
                parameter: 'sigma',
                min: 0.5,
                max: 10,
                default: 2,
                step: 0.5,
                icon: '🌫️'
            },
            {
                id: 'sharpen',
                name: 'Sharpen',
                description: 'Sharpen the image',
                hasSlider: true,
                parameter: 'intensity',
                min: 0.1,
                max: 2.0,
                default: 1.0,
                step: 0.1,
                icon: '⚡'
            },
            {
                id: 'noise',
                name: 'Grainy',
                description: 'Add noise/grain effect',
                hasSlider: true,
                parameter: 'level',
                min: 5,
                max: 100,
                default: 25,
                step: 5,
                icon: '✨'
            },
            {
                id: 'edges',
                name: 'Edge Detection',
                description: 'Detect edges using Sobel operator',
                hasSlider: false,
                icon: '🔲'
            },
            {
                id: 'compress',
                name: 'Compress',
                description: 'Multi-level image compression',
                hasSlider: true,
                parameter: 'levels',
                min: 1,
                max: 5,
                default: 3,
                step: 1,
                icon: '📦'
            },
            {
                id: 'brightness',
                name: 'Brightness',
                description: 'Adjust brightness/exposure',
                hasSlider: true,
                parameter: 'value',
                min: -100,
                max: 100,
                default: 0,
                step: 5,
                icon: '☀️'
            },
            {
                id: 'saturation',
                name: 'Saturation',
                description: 'Adjust color saturation',
                hasSlider: true,
                parameter: 'value',
                min: 0,
                max: 2,
                default: 1,
                step: 0.1,
                icon: '🎨'
            },
            {
                id: 'flip-h',
                name: 'Flip Horizontal',
                description: 'Flip image horizontally',
                hasSlider: false,
                icon: '↔️'
            },
            {
                id: 'flip-v',
                name: 'Flip Vertical',
                description: 'Flip image vertically',
                hasSlider: false,
                icon: '↕️'
            },
            {
                id: 'rotate90',
                name: 'Rotate 90°',
                description: 'Rotate in 90° increments',
                hasSlider: true,
                parameter: 'times',
                min: 1,
                max: 3,
                default: 1,
                step: 1,
                icon: '🔄'
            },
            {
                id: 'rotate',
                name: 'Rotate (Angle)',
                description: 'Rotate by specific angle',
                hasSlider: true,
                parameter: 'angle',
                min: 0,
                max: 360,
                default: 45,
                step: 15,
                icon: '🔃'
            }
        ]
    });
});

// Error handling middleware
app.use((err, req, res, next) => {
    console.error('Error:', err);
    res.status(500).json({ error: err.message || 'Internal server error' });
});

// Start server
app.listen(PORT, () => {
    console.log(`
╔═══════════════════════════════════════════╗
║  🚀 HPC Image Processor Server Running   ║
║  📍 Port: ${PORT}                            ║
║  🌐 http://localhost:${PORT}                ║
╚═══════════════════════════════════════════╝
    `);
});
