# Merkle Tree Integrity Verification System
## Complete GUI Implementation

---

## 🎯 Project Overview

A **professional, production-grade GUI** for visualizing and interacting with a Merkle Tree integrity verification system. Built with modern web technologies and C++ backend.

### Key Features

✨ **Visual Tree Representation**
- Interactive D3.js quad-tree visualization
- Zoom, pan, and navigation controls
- Color-coded nodes (root/internal/leaf/proof path)
- SVG export functionality

🎬 **Animated Proof Paths**
- Step-by-step proof generation
- Real-time verification animation
- Hash concatenation visualization

📊 **Interactive Dashboard**
- Real-time performance metrics
- Memory usage tracking
- Root hash history
- Integrity status monitoring

---

## 📁 Project Structure

```
AlgoProject/
├── backend/                    # C++ Backend
│   ├── merkle.cpp             # Core Merkle Tree implementation
│   ├── api_server.cpp         # HTTP REST API server
│   ├── merkle_api.cpp         # API logic layer
│   ├── merkle_api.hpp         # API header
│   ├── httplib.h              # HTTP server library
│   ├── sha.cpp/hpp            # SHA-256 implementation
│   ├── json.hpp               # JSON parser
│   └── API_README.md          # Backend documentation
│
├── frontend/                   # React Frontend
│   ├── src/
│   │   ├── components/
│   │   │   ├── Dashboard/              # Metrics dashboard
│   │   │   ├── TreeVisualization/      # D3.js tree
│   │   │   ├── ProofVisualization/     # Proof generator
│   │   │   ├── DatasetSelector/        # Dataset UI
│   │   │   └── Layout/                 # Main layout
│   │   ├── services/api.ts             # API client
│   │   ├── stores/merkleStore.ts       # State management
│   │   ├── types/merkle.types.ts       # TypeScript types
│   │   └── lib/utils.ts                # Utilities
│   ├── package.json
│   ├── tailwind.config.js
│   └── README.md
│
├── Dataset/                    # JSON datasets
└── MerkleTreeLogic/           # Original implementation
```

---

## 🚀 Quick Start Guide

### Prerequisites

- **Node.js 18+** and npm
- **g++** with C++11 support
- **Linux/Unix/MacOS** (for MMAP support)

### Step 1: Build Backend API Server

```bash
cd backend

# Download HTTP library
curl -o httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h

# Compile API server
g++ -std=c++11 -O3 -o api_server api_server.cpp merkle_api.cpp sha.cpp -lpthread

# Run server
./api_server
```

Server will start on `http://localhost:8080`

### Step 2: Start Frontend

```bash
cd frontend

# Install dependencies (if not already done)
npm install

# Start development server
npm run dev
```

Frontend will run on `http://localhost:5173`

### Step 3: Use the Application

1. **Select Dataset**: Choose from available JSON datasets
2. **Build Tree**: Click to build Merkle Tree (may take 5-10s for large datasets)
3. **View Dashboard**: See real-time metrics and integrity status
4. **Explore Tree**: Zoom/pan the interactive tree visualization
5. **Generate Proofs**: Enter a record ID to generate and verify cryptographic proofs

---

## 🛠️ Technology Stack

### Backend
- **C++11** - Core implementation
- **cpp-httplib** - HTTP server
- **nlohmann/json** - JSON parsing
- **SHA-256** - Cryptographic hashing
- **MMAP** - Zero-copy file I/O
- **Parallel Processing** - Multi-threaded tree building

### Frontend
- **React 18** - UI framework
- **TypeScript** - Type safety
- **D3.js v7** - Tree visualization
- **Tailwind CSS** - Styling
- **Framer Motion** - Animations
- **Recharts** - Charts
- **Zustand** - State management
- **Axios** - HTTP client
- **Vite 5** - Build tool

---

## 📊 Performance Metrics

| Metric | Value | Target | Status |
|--------|-------|--------|--------|
| Dataset Size | 1,512,530 records | 1M+ | ✅ 151% |
| Build Time | 8.67s | N/A | ✅ Fast |
| Proof Latency | 0.051ms | <100ms | ✅ 2000x |
| Memory Usage | 1825 MB | N/A | ✅ Tracked |
| Frontend Bundle | 389 KB | <500KB | ✅ Optimized |
| Tree Render | <3s | <5s | ✅ Smooth |

---

## 🎨 Design System

### Color Palette

- **Primary (Blue)**: `#3b82f6` - Interactive elements, tree nodes
- **Success (Green)**: `#10b981` - Verified states, leaf nodes
- **Warning (Orange)**: `#f59e0b` - Proof paths, alerts
- **Danger (Red)**: `#ef4444` - Errors, tampered data
- **Gold**: `#fbbf24` - Root node
- **Dark**: `#0f172a` - Background

### UI Components

- **Glassmorphism Cards**: Frosted glass effect with backdrop blur
- **Gradient Text**: Smooth color transitions
- **Glow Effects**: Subtle shadows for emphasis
- **Smooth Animations**: 60fps transitions

---

## 🔌 API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/health` | Health check |
| GET | `/api/datasets` | List available datasets |
| POST | `/api/tree/build` | Build tree for dataset |
| GET | `/api/tree/structure` | Get tree nodes (limited depth) |
| GET | `/api/tree/root` | Get current root hash |
| POST | `/api/proof/generate` | Generate proof for record ID |
| POST | `/api/proof/verify` | Verify proof |
| GET | `/api/stats` | Get performance metrics |
| GET | `/api/history` | Get root hash history |

---

## 🧪 Testing

### Backend Testing

```bash
cd backend

# Test API health
curl http://localhost:8080/api/health

# List datasets
curl http://localhost:8080/api/datasets

# Build tree
curl -X POST http://localhost:8080/api/tree/build \
  -H "Content-Type: application/json" \
  -d '{"dataset":"Musical_Instruments.json"}'

# Generate proof
curl -X POST http://localhost:8080/api/proof/generate \
  -H "Content-Type: application/json" \
  -d '{"recordId":"B001234_0_42"}'
```

### Frontend Testing

```bash
cd frontend

# Build for production
npm run build

# Preview production build
npm run preview

# Lint code
npm run lint
```

---

## 📸 Screenshots

### Dashboard
- Real-time metrics (records, build time, memory, integrity)
- Root hash display with historical comparison
- Performance insights and tree statistics

### Tree Visualization
- Interactive D3.js quad-tree
- Zoom/pan controls
- Color-coded nodes
- Proof path highlighting

### Proof Generator
- Record ID input
- Animated proof generation
- Step-by-step verification
- Hash display with formatting

---

## 🎓 Academic Context

This project demonstrates:

1. **Algorithmic Optimization**
   - Quad-tree vs binary tree (50% depth reduction)
   - Batch update algorithm (30,000x speedup)
   - Parallel processing (3-4x faster)

2. **Data Structure Design**
   - Flat array storage (cache efficiency)
   - Raw byte hashing (66% memory reduction)
   - MMAP zero-copy I/O

3. **Full-Stack Development**
   - C++ backend with HTTP API
   - React frontend with D3.js
   - Real-time data visualization

4. **Cryptographic Integrity**
   - SHA-256 hashing
   - Merkle proof generation
   - Tamper detection

---

## 🐛 Troubleshooting

### Backend Issues

**Problem**: `MMAP failed`
- **Solution**: Ensure dataset files exist in `../Dataset/` directory

**Problem**: `Port 8080 already in use`
- **Solution**: Kill existing process or change port in `api_server.cpp`

### Frontend Issues

**Problem**: `Cannot connect to API`
- **Solution**: Ensure backend server is running on `http://localhost:8080`

**Problem**: `Tree not rendering`
- **Solution**: Build a tree first by selecting a dataset

**Problem**: `Node.js version error`
- **Solution**: Use Node.js 18+ (current implementation uses Vite 5)

---

## 📝 Future Enhancements

- [ ] Real-time WebSocket updates
- [ ] Batch proof generation
- [ ] Tree comparison view (before/after)
- [ ] PDF report generation
- [ ] Mobile responsive design improvements
- [ ] Dark/light theme toggle
- [ ] Historical root timeline chart
- [ ] Export tree as PNG/JSON

---

## 👨‍💻 Development

### Adding New Features

1. **Backend**: Add endpoint in `api_server.cpp`, implement logic in `merkle_api.cpp`
2. **Frontend**: Add API method in `services/api.ts`, create component, update store

### Code Style

- **Backend**: Follow C++11 standards, use meaningful variable names
- **Frontend**: Use TypeScript, follow React hooks patterns, Tailwind for styling

---

## 📄 License

Part of the Merkle Tree Integrity Verification System academic project.

---

## 🙏 Acknowledgments

- **cpp-httplib**: Lightweight HTTP server library
- **D3.js**: Powerful data visualization library
- **Tailwind CSS**: Utility-first CSS framework
- **React**: Modern UI framework

---

**Built with ❤️ for academic excellence and professional quality**
