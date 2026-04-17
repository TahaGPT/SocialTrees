// API Configuration
export const API_CONFIG = {
    BASE_URL: import.meta.env.VITE_API_BASE_URL || 'http://localhost:8080/api',
    TIMEOUT: parseInt(import.meta.env.VITE_API_TIMEOUT || '30000'),
    HEALTH_CHECK_TIMEOUT: 5000,
    DATASETS_TIMEOUT: 10000,
};

// Animation Configuration
export const ANIMATION_CONFIG = {
    DURATION: parseInt(import.meta.env.VITE_ANIMATION_DURATION || '500'),
    HIGHLIGHT_DURATION: parseInt(import.meta.env.VITE_HIGHLIGHT_DURATION || '3000'),
    TRANSITION_DURATION: 200,
    PROOF_DELAY: 800,
    BUILD_PROGRESS_CLEAR_DELAY: 2000,
};

// Tree Configuration
export const TREE_CONFIG = {
    ARITY: 4, // Quad-tree
    DEFAULT_MAX_DEPTH: parseInt(import.meta.env.VITE_DEFAULT_TREE_DEPTH || '10'),
    DEFAULT_RECORD_COUNT: 40, // For fallback calculations
    DEFAULT_LEAF_CAPACITY: 64,
};

// Performance Target
export const PERFORMANCE_TARGETS = {
    PROOF_GENERATION_MAX_MS: 100, // Target: proof generation + verification < 100ms
};

// Sample Data (for demo/testing purposes)
export const SAMPLE_CONFIG = {
    ASIN: '0470536454', // Musical Instruments dataset ASIN
    DEFAULT_RATING: 5.0,
    DEFAULT_TEXT: 'New inserted record',
};

// UI Configuration
export const UI_CONFIG = {
    MAX_VISIBLE_HISTORY: 10,
    TRUNCATE_HASH_LENGTH: 16,
    BYTES_PER_KB: 1024,
};
