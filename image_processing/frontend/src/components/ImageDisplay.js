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
          ⚡ Processed in {processingTime}ms
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
                <div className="slider-handle">↔️</div>
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
            ⬇️ Download Processed Image
          </button>
        </div>
      )}
    </div>
  );
}

export default ImageDisplay;
