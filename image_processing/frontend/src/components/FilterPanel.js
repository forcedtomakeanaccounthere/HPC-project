import React from 'react';
import './FilterPanel.css';

const FilterIcons = {
  grayscale: (
    <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
      <circle cx="12" cy="12" r="10"/>
      <path d="M12 2 A10 10 0 0 1 12 22"/>
    </svg>
  ),
  blur: (
    <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
      <circle cx="12" cy="12" r="3"/>
      <circle cx="12" cy="12" r="7" opacity="0.5"/>
      <circle cx="12" cy="12" r="10" opacity="0.3"/>
    </svg>
  ),
  sharpen: (
    <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
      <polygon points="12,2 22,12 12,22 2,12"/>
      <line x1="12" y1="8" x2="12" y2="16"/>
      <line x1="8" y1="12" x2="16" y2="12"/>
    </svg>
  ),
  noise: (
    <svg width="32" height="32" viewBox="0 0 24 24" fill="currentColor">
      <circle cx="5" cy="5" r="1.5"/>
      <circle cx="12" cy="3" r="1.5"/>
      <circle cx="19" cy="6" r="1.5"/>
      <circle cx="3" cy="12" r="1.5"/>
      <circle cx="9" cy="10" r="1.5"/>
      <circle cx="15" cy="13" r="1.5"/>
      <circle cx="21" cy="11" r="1.5"/>
      <circle cx="7" cy="18" r="1.5"/>
      <circle cx="13" cy="20" r="1.5"/>
      <circle cx="18" cy="17" r="1.5"/>
    </svg>
  ),
  edges: (
    <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
      <rect x="3" y="3" width="18" height="18" rx="2"/>
      <path d="M3 9 L9 3 M9 21 L3 15 M15 3 L21 9 M21 15 L15 21"/>
    </svg>
  ),
  compress: (
    <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
      <path d="M8 3 L8 8 L3 8 M16 3 L16 8 L21 8 M8 21 L8 16 L3 16 M16 21 L16 16 L21 16"/>
      <rect x="9" y="9" width="6" height="6"/>
    </svg>
  ),
  brightness: (
    <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
      <circle cx="12" cy="12" r="5"/>
      <line x1="12" y1="1" x2="12" y2="3"/>
      <line x1="12" y1="21" x2="12" y2="23"/>
      <line x1="4.22" y1="4.22" x2="5.64" y2="5.64"/>
      <line x1="18.36" y1="18.36" x2="19.78" y2="19.78"/>
      <line x1="1" y1="12" x2="3" y2="12"/>
      <line x1="21" y1="12" x2="23" y2="12"/>
      <line x1="4.22" y1="19.78" x2="5.64" y2="18.36"/>
      <line x1="18.36" y1="5.64" x2="19.78" y2="4.22"/>
    </svg>
  ),
  saturation: (
    <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
      <path d="M12 2.69l5.66 5.66a8 8 0 1 1-11.31 0z"/>
      <path d="M12 2v20" opacity="0.5"/>
    </svg>
  ),
  'flip-h': (
    <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
      <path d="M12 2 L12 22 M8 6 L2 12 L8 18 M16 6 L22 12 L16 18"/>
    </svg>
  ),
  'flip-v': (
    <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
      <path d="M2 12 L22 12 M6 8 L12 2 L18 8 M6 16 L12 22 L18 16"/>
    </svg>
  ),
  rotate90: (
    <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
      <path d="M21.5 2v6h-6M2.5 22v-6h6M2 11.5a10 10 0 0 1 18.8-4.3M22 12.5a10 10 0 0 1-18.8 4.2"/>
    </svg>
  ),
  rotate: (
    <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
      <path d="M21 12a9 9 0 1 1-9-9c2.52 0 4.93 1 6.74 2.74L21 8"/>
      <path d="M21 3v5h-5"/>
    </svg>
  )
};

function FilterPanel({ filters, selectedFilter, onFilterSelect }) {
  return (
    <div className="filter-panel">
      <h2>Select Filter</h2>
      <div className="filters-grid">
        {filters.map((filter) => (
          <div
            key={filter.id}
            className={`filter-card ${selectedFilter?.id === filter.id ? 'selected' : ''}`}
            onClick={() => onFilterSelect(filter)}
          >
            <div className="filter-icon">
              {FilterIcons[filter.id] || filter.icon}
            </div>
            <div className="filter-name">{filter.name}</div>
          </div>
        ))}
      </div>
    </div>
  );
}

export default FilterPanel;
