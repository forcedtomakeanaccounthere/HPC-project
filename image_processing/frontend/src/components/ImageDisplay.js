import React, { useState } from 'react';
import './ImageDisplay.css';

function ImageDisplay({ originalImage, processedImage, processingTime, onDownload }) {
  const [compareMode, setCompareMode] = useState(false);
  const [sliderPosition, setSliderPosition] = useState(50);

  if (!originalImage) {
    return (
      <div className="image-display-empty">
        <div className="empty-state">
          <div className="empty-icon">🖼️</div>
          <h3>No image uploaded</h3>
          <p>Upload an image to get started</p>
        </div>
      </div>
    );
  }

  return (
    <div className="image-display">
      <div className="display-header">
        <h2>🖼️ Image Preview</h2>
        {processedImage && (
          <div className="display-controls">
            <button
              className={`view-toggle ${!compareMode ? 'active' : ''}`}
              onClick={() => setCompareMode(false)}
            >
              Split View
            </button>
            <button
              className={`view-toggle ${compareMode ? 'active' : ''}`}
              onClick={() => setCompareMode(true)}
            >
              Compare Slider
            </button>
          </div>
        )}
      </div>

      {processingTime && (
        <div className="performance-badge">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor" style={{display: 'inline-block', verticalAlign: 'middle', marginRight: '6px'}}>
            <polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/>
          </svg>
          Processed in {processingTime}ms
          <span className="performance-details">
            {' '}({(processingTime / 1000).toFixed(2)}s)
          </span>
        </div>
      )}

      {!compareMode || !processedImage ? (
        <div className="images-split-view">
          <div className="image-box">
            <h3>Original</h3>
            <div className="image-wrapper">
              <img src={originalImage.path} alt="Original" />
              <div className="image-info">
                {originalImage.originalName}
                <br />
                {(originalImage.size / 1024).toFixed(2)} KB
              </div>
            </div>
          </div>

          {processedImage && (
            <div className="image-box">
              <h3>Processed</h3>
              <div className="image-wrapper">
                <img src={processedImage.path} alt="Processed" />
                <div className="image-info">
                  Filter: {processedImage.filter}
                  {processedImage.params && Object.keys(processedImage.params).length > 0 && (
                    <>
                      <br />
                      {Object.entries(processedImage.params).map(([key, value]) => (
                        <span key={key}>
                          {key}: {typeof value === 'number' ? value.toFixed(2) : value}
                        </span>
                      ))}
                    </>
                  )}
                </div>
              </div>
            </div>
          )}
        </div>
      ) : (
        <div className="images-compare-view">
          <div className="compare-container">
            <div className="compare-wrapper">
              <img src={originalImage.path} alt="Original" className="compare-image original" />
              <img
                src={processedImage.path}
                alt="Processed"
                className="compare-image processed"
                style={{ clipPath: `inset(0 ${100 - sliderPosition}% 0 0)` }}
              />
              <div
                className="compare-slider"
                style={{ left: `${sliderPosition}%` }}
              >
                <div className="slider-handle">
                  <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                    <line x1="3" y1="12" x2="21" y2="12"/>
                    <polyline points="8 7 3 12 8 17"/>
                    <polyline points="16 7 21 12 16 17"/>
                  </svg>
                </div>
              </div>
              <input
                type="range"
                min="0"
                max="100"
                value={sliderPosition}
                onChange={(e) => setSliderPosition(e.target.value)}
                className="compare-range"
              />
            </div>
            <div className="compare-labels">
              <span>Processed</span>
              <span>Original</span>
            </div>
          </div>
        </div>
      )}

      {processedImage && (
        <div className="action-bar">
          <button className="download-button" onClick={onDownload}>
            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" style={{display: 'inline-block', verticalAlign: 'middle', marginRight: '8px'}}>
              <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/>
              <polyline points="7 10 12 15 17 10"/>
              <line x1="12" y1="15" x2="12" y2="3"/>
            </svg>
            Download Processed Image
          </button>
        </div>
      )}
    </div>
  );
}

export default ImageDisplay;
