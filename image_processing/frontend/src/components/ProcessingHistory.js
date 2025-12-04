import React from 'react';
import './ProcessingHistory.css';

function ProcessingHistory({ history, onClose }) {
  return (
    <div className="history-modal">
      <div className="history-container">
        <div className="history-header">
          <h2>📜 Processing History</h2>
          <button className="close-button" onClick={onClose}>✕</button>
        </div>
        
        <div className="history-content">
          {history.length === 0 ? (
            <div className="history-empty">
              <p>No processing history yet</p>
              <p>Upload and process images to see them here</p>
            </div>
          ) : (
            <div className="history-list">
              {history.map((item) => (
                <div key={item.id} className="history-item">
                  <div className="history-images">
                    {item.uploadedPath && (
                      <img
                        src={`http://localhost:5000${item.uploadedPath}`}
                        alt="Original"
                        className="history-thumbnail"
                      />
                    )}
                    {item.processedPath && (
                      <img
                        src={`http://localhost:5000${item.processedPath}`}
                        alt="Processed"
                        className="history-thumbnail"
                      />
                    )}
                  </div>
                  <div className="history-details">
                    <div className="history-filename">{item.originalName}</div>
                    <div className="history-filter">
                      <span className="filter-badge">{item.filter || 'Unknown'}</span>
                      {item.parameters && Object.keys(item.parameters).length > 0 && (
                        <span className="params-badge">
                          {Object.entries(item.parameters).map(([key, value]) => (
                            <span key={key}>
                              {key}: {typeof value === 'number' ? value.toFixed(2) : value}
                            </span>
                          ))}
                        </span>
                      )}
                    </div>
                    <div className="history-meta">
                      <span>⏱️ {item.processingTime}ms</span>
                      <span>📅 {new Date(item.createdAt).toLocaleString()}</span>
                    </div>
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

export default ProcessingHistory;
