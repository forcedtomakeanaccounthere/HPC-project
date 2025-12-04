import React, { useState, useEffect } from 'react';
import './App.css';
import ImageUpload from './components/ImageUpload';
import FilterPanel from './components/FilterPanel';
import ImageDisplay from './components/ImageDisplay';
import ProcessingHistory from './components/ProcessingHistory';

const API_URL = 'http://localhost:5000/api';

function App() {
  const [uploadedImage, setUploadedImage] = useState(null);
  const [processedImage, setProcessedImage] = useState(null);
  const [filters, setFilters] = useState([]);
  const [selectedFilter, setSelectedFilter] = useState(null);
  const [filterParams, setFilterParams] = useState({});
  const [processing, setProcessing] = useState(false);
  const [processingTime, setProcessingTime] = useState(null);
  const [history, setHistory] = useState([]);
  const [showHistory, setShowHistory] = useState(false);
  const [error, setError] = useState(null);

  // Load available filters
  useEffect(() => {
    fetchFilters();
    fetchHistory();
  }, []);

  const fetchFilters = async () => {
    try {
      const response = await fetch(`${API_URL}/filters`);
      const data = await response.json();
      if (data.success) {
        setFilters(data.filters);
      }
    } catch (error) {
      console.error('Failed to fetch filters:', error);
    }
  };

  const fetchHistory = async () => {
    try {
      const response = await fetch(`${API_URL}/history`);
      const data = await response.json();
      if (data.success) {
        setHistory(data.history);
      }
    } catch (error) {
      console.error('Failed to fetch history:', error);
    }
  };

  const handleImageUpload = (imageData) => {
    setUploadedImage(imageData);
    setProcessedImage(null);
    setProcessingTime(null);
    setError(null);
  };

  const handleFilterSelect = (filter) => {
    setSelectedFilter(filter);
    // Set default parameters
    if (filter.hasSlider) {
      setFilterParams({ [filter.parameter]: filter.default });
    } else {
      setFilterParams({});
    }
  };

  const handleParamChange = (paramName, value) => {
    setFilterParams(prev => ({
      ...prev,
      [paramName]: value
    }));
  };

  const handleApplyFilter = async () => {
    if (!uploadedImage || !selectedFilter) {
      setError('Please upload an image and select a filter');
      return;
    }

    setProcessing(true);
    setError(null);

    try {
      const response = await fetch(`${API_URL}/process`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          filename: uploadedImage.filename,
          filter: selectedFilter.id,
          parameters: filterParams
        }),
      });

      const data = await response.json();

      if (data.success) {
        setProcessedImage({
          path: `http://localhost:5000${data.result.processedPath}`,
          filter: selectedFilter.name,
          params: filterParams
        });
        setProcessingTime(data.result.processingTime);
        fetchHistory(); // Refresh history
      } else {
        setError(data.error || 'Processing failed');
      }
    } catch (error) {
      console.error('Processing error:', error);
      setError('Failed to process image. Make sure the backend server is running.');
    } finally {
      setProcessing(false);
    }
  };

  const handleReset = () => {
    setUploadedImage(null);
    setProcessedImage(null);
    setSelectedFilter(null);
    setFilterParams({});
    setProcessingTime(null);
    setError(null);
  };

  const handleDownload = () => {
    if (processedImage) {
      const link = document.createElement('a');
      link.href = processedImage.path;
      link.download = `processed-image-${Date.now()}.png`;
      link.click();
    }
  };

  return (
    <div className="App">
      <header className="app-header">
        <h1>🚀 HPC Image Processor</h1>
        <p>High-Performance Computing powered image editing with OpenMP + CUDA</p>
      </header>

      {error && (
        <div className="error-banner">
          <span>⚠️ {error}</span>
          <button onClick={() => setError(null)}>✕</button>
        </div>
      )}

      <div className="main-container">
        {/* Filter Selection Section at Top */}
        {uploadedImage && (
          <div className="filter-section">
            <FilterPanel
              filters={filters}
              selectedFilter={selectedFilter}
              filterParams={filterParams}
              onFilterSelect={handleFilterSelect}
              onParamChange={handleParamChange}
            />
          </div>
        )}

        {/* Controls Row */}
        {uploadedImage && (
          <div className="controls-row">
            {selectedFilter && selectedFilter.hasSlider && (
              <div className="intensity-control">
                <label htmlFor={selectedFilter.parameter}>
                  Adjust {selectedFilter.parameter}
                </label>
                <input
                  type="range"
                  id={selectedFilter.parameter}
                  min={selectedFilter.min}
                  max={selectedFilter.max}
                  step={selectedFilter.step}
                  value={filterParams[selectedFilter.parameter] || selectedFilter.default}
                  onChange={(e) => handleParamChange(selectedFilter.parameter, parseFloat(e.target.value))}
                  className="intensity-slider"
                />
                <span className="slider-value-display">
                  {filterParams[selectedFilter.parameter] !== undefined 
                    ? filterParams[selectedFilter.parameter].toFixed(selectedFilter.step < 1 ? 1 : 0)
                    : selectedFilter.default}
                </span>
              </div>
            )}
            <div className="action-buttons-row">
              <button 
                className="apply-button"
                onClick={handleApplyFilter}
                disabled={processing || !selectedFilter}
              >
                {processing ? '⏳ Processing...' : '✨ Apply Filter'}
              </button>
              <button 
                className="reset-button"
                onClick={handleReset}
              >
                🔄 Reset
              </button>
              <button 
                className="history-button"
                onClick={() => setShowHistory(!showHistory)}
              >
                {showHistory ? '📷 Hide' : '📜 Show'} History
              </button>
            </div>
          </div>
        )}

        {/* Upload and Preview Side by Side */}
        <div className="content-row">
          <div className="upload-section">
            <ImageUpload onImageUpload={handleImageUpload} />
          </div>
          <div className="preview-section">
            <ImageDisplay
              originalImage={uploadedImage}
              processedImage={processedImage}
              processingTime={processingTime}
              onDownload={handleDownload}
            />
          </div>
        </div>
      </div>

      {showHistory && (
        <ProcessingHistory
          history={history}
          onClose={() => setShowHistory(false)}
        />
      )}

      <footer className="app-footer">
        <p>Powered by parallel processing: OpenMP for CPU, CUDA for GPU</p>
        <p>Real-time performance metrics displayed</p>
      </footer>
    </div>
  );
}

export default App;
