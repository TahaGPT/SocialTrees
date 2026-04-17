# Configuration Guide

This project has been refactored to remove hardcoded values and use configuration files.

## Frontend Configuration

### Environment Variables (.env)
Located in `/frontend/.env`

```bash
# API Configuration
VITE_API_BASE_URL=http://localhost:8080/api
VITE_API_TIMEOUT=30000

# Animation Configuration
VITE_ANIMATION_DURATION=500
VITE_HIGHLIGHT_DURATION=3000

# Tree Configuration
VITE_DEFAULT_TREE_DEPTH=10
```

### Constants File
Located in `/frontend/src/config/constants.ts`

- **API_CONFIG**: Base URL, timeouts
- **ANIMATION_CONFIG**: Animation durations, transitions
- **TREE_CONFIG**: Tree arity (4 for quad-tree), depth, capacity
- **PERFORMANCE_TARGETS**: Performance goals (e.g., <100ms for proofs)
- **SAMPLE_CONFIG**: Default values for testing (ASIN, ratings)
- **UI_CONFIG**: UI-specific settings (truncation lengths, display limits)

## Backend Configuration

### Config Header
Located in `/backend/config.h`

```cpp
namespace Config {
    const std::string SERVER_HOST = "0.0.0.0";
    const int SERVER_PORT = 8080;
    const std::string DATASET_DIR = "../Dataset/";
    const std::string ROOT_HISTORY_FILE = "merkle_roots.txt";
    const int TREE_ARITY = 4; // Quad-tree
    const int DEFAULT_MAX_DEPTH = 10;
    const int DEFAULT_THREAD_COUNT = 12;
}
```

## Removed Hardcoded Values

### Frontend
✅ API URL (http://localhost:8080)
✅ Timeouts (30000ms, 5000ms, 10000ms)
✅ Animation durations (500ms, 3000ms, 200ms, 800ms, 2000ms)
✅ Tree arity (4)
✅ Leaf capacity calculations (64, 40)
✅ Sample ASIN (0470536454)
✅ Default ratings (5.0)
✅ Performance targets (100ms)

### Backend
✅ Server host (0.0.0.0)
✅ Server port (8080)
✅ Dataset directory (../Dataset/)
✅ Root history file (merkle_roots.txt)
✅ Tree arity (4)

## Benefits

1. **Easy Configuration**: Change settings in one place
2. **Environment-Specific**: Use different .env files for dev/prod
3. **Type Safety**: Constants with proper types
4. **Maintainability**: No magic numbers scattered in code
5. **Documentation**: Clear purpose for each configuration value

## How to Modify

### Change API URL:
Edit `frontend/.env`:
```bash
VITE_API_BASE_URL=http://your-server:3000/api
```

### Change Server Port:
Edit `backend/config.h`:
```cpp
const int SERVER_PORT = 3000;
```

### Change Animation Speed:
Edit `frontend/.env`:
```bash
VITE_ANIMATION_DURATION=1000  # Slower animations
VITE_HIGHLIGHT_DURATION=5000  # Longer highlight duration
```

### Change Performance Target:
Edit `frontend/src/config/constants.ts`:
```typescript
export const PERFORMANCE_TARGETS = {
    PROOF_GENERATION_MAX_MS: 50, // More aggressive target
};
```
