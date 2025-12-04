import React from 'react';
import './ProcessingHistory.css';

function ProcessingHistory({ history, onClose }) {
  return (
    <div className="history-modal">
      <div className="history-container">
        <div className="history-header">
          <h2>
            <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" style={{display: 'inline-block', verticalAlign: 'middle', marginRight: '8px'}}>
              <path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/>
              <polyline points="14 2 14 8 20 8"/>
              <line x1="16" y1="13" x2="8" y2="13"/>
              <line x1="16" y1="17" x2="8" y2="17"/>
              <polyline points="10 9 9 9 8 9"/>
            </svg>
            Processing History
          </h2>
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
                      <span>
                        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" style={{display: 'inline-block', verticalAlign: 'middle', marginRight: '4px'}}>
                          <circle cx="12" cy="12" r="10"/>
                          <polyline points="12 6 12 12 16 14"/>
                        </svg>
                        {item.processingTime}ms
                      </span>
                      <span>
                        <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" style={{display: 'inline-block', verticalAlign: 'middle', marginRight: '4px'}}>
                          <rect x="3" y="4" width="18" height="18" rx="2" ry="2"/>
                          <line x1="16" y1="2" x2="16" y2="6"/>
                          <line x1="8" y1="2" x2="8" y2="6"/>
                          <line x1="3" y1="10" x2="21" y2="10"/>
                        </svg>
                        {new Date(item.createdAt).toLocaleString()}
                      </span>
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
