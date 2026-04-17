# Merkle Tree GUI - Frontend

Professional React + TypeScript + D3.js frontend for Merkle Tree visualization.

## Features

✨ **Visual Tree Representation**
- Interactive D3.js quad-tree visualization
- Zoom, pan, and navigation controls
- Color-coded nodes (root, internal, leaf, proof path)
- SVG export functionality

🎬 **Animated Proof Paths**
- Step-by-step proof generation
- Real-time verification animation
- Hash concatenation visualization
- Proof timeline with playback

📊 **Interactive Dashboard**
- Real-time performance metrics
- Memory usage tracking
- Root hash history
- Integrity status monitoring

## Tech Stack

- **React 18** - UI framework
- **TypeScript** - Type safety
- **D3.js** - Tree visualization
- **Tailwind CSS** - Styling
- **Framer Motion** - Animations
- **Recharts** - Charts
- **Zustand** - State management
- **Axios** - API client
- **Vite** - Build tool

## Getting Started

### Prerequisites

- Node.js 18+ and npm
- Backend API server running on `http://localhost:8080`

### Installation

```bash
# Install dependencies
npm install

# Start development server
npm run dev

# Build for production
npm run build

# Preview production build
npm run preview
```

### Development

The app will run on `http://localhost:5173` by default.

Make sure the backend API server is running before starting the frontend.

## Project Structure

```
src/
├── components/
│   ├── Dashboard/          # Metrics and statistics
│   ├── DatasetSelector/    # Dataset selection UI
│   ├── Layout/             # Main layout components
│   ├── ProofVisualization/ # Proof generation and verification
│   └── TreeVisualization/  # D3.js tree rendering
├── lib/
│   └── utils.ts            # Utility functions
├── pages/
│   └── HomePage.tsx        # Main page
├── services/
│   └── api.ts              # API client
├── stores/
│   └── merkleStore.ts      # Global state
├── types/
│   └── merkle.types.ts     # TypeScript types
├── App.tsx                 # App root
├── main.tsx                # Entry point
└── index.css               # Global styles
```

## Design System

### Colors

- **Primary (Blue)**: Tree nodes, interactive elements
- **Success (Green)**: Verified states, leaf nodes
- **Warning (Orange)**: Proof paths, alerts
- **Danger (Red)**: Errors, tampered data
- **Gold**: Root node
- **Dark**: Background, surfaces

### Components

- **Glass Cards**: Glassmorphism effect with backdrop blur
- **Buttons**: Primary, secondary, success, danger variants
- **Inputs**: Consistent styling with focus states
- **Hash Display**: Monospace font with syntax highlighting

## API Integration

The frontend communicates with the C++ backend via REST API:

- `GET /api/datasets` - List datasets
- `POST /api/tree/build` - Build tree
- `GET /api/tree/structure` - Get tree nodes
- `POST /api/proof/generate` - Generate proof
- `POST /api/proof/verify` - Verify proof
- `GET /api/stats` - Get metrics
- `GET /api/history` - Get root history

## Performance

- Tree rendering: <3s for 1.5M records
- Proof animation: 60fps smooth
- Dashboard updates: <100ms latency
- Memory usage: <500MB in browser

## Browser Support

- Chrome/Edge 90+
- Firefox 88+
- Safari 14+

## License

Part of the Merkle Tree Integrity Verification System project.
