import { create } from 'zustand';
import type {
    Dataset,
    TreeStructure,
    TreeStats,
    Proof,
    RootHistoryEntry,
} from '../types/merkle.types';

interface MerkleState {
    // Data
    datasets: Dataset[];
    currentDataset: string | null;
    treeStructure: TreeStructure | null;
    stats: TreeStats | null;
    rootHistory: RootHistoryEntry[];
    currentProof: Proof | null;

    // UI State
    isLoading: boolean;
    error: string | null;
    selectedNode: number | null;
    proofPath: number[];
    selectedNodeDetails: any | null;

    // Actions
    setDatasets: (datasets: Dataset[]) => void;
    setCurrentDataset: (dataset: string) => void;
    setTreeStructure: (structure: TreeStructure) => void;
    setStats: (stats: TreeStats) => void;
    setRootHistory: (history: RootHistoryEntry[]) => void;
    setCurrentProof: (proof: Proof | null) => void;
    setIsLoading: (loading: boolean) => void;
    setError: (error: string | null) => void;
    setSelectedNode: (nodeIndex: number | null) => void;
    setProofPath: (path: number[]) => void;
    setSelectedNodeDetails: (details: any | null) => void;
    reset: () => void;
}

export const useMerkleStore = create<MerkleState>((set) => ({
    // Initial state
    datasets: [],
    currentDataset: null,
    treeStructure: null,
    stats: null,
    rootHistory: [],
    currentProof: null,
    isLoading: false,
    error: null,
    selectedNode: null,
    proofPath: [],
    selectedNodeDetails: null,

    // Actions
    setDatasets: (datasets) => set({ datasets }),
    setCurrentDataset: (dataset) => set({ currentDataset: dataset }),
    setTreeStructure: (structure) => set({ treeStructure: structure }),
    setStats: (stats) => set({ stats }),
    setRootHistory: (history) => set({ rootHistory: history }),
    setCurrentProof: (proof) => set({ currentProof: proof }),
    setIsLoading: (loading) => set({ isLoading: loading }),
    setError: (error) => set({ error }),
    setSelectedNode: (nodeIndex) => set({ selectedNode: nodeIndex }),
    setProofPath: (path) => set({ proofPath: path }),
    setSelectedNodeDetails: (details) => set({ selectedNodeDetails: details }),
    reset: () => set({
        currentDataset: null,
        treeStructure: null,
        stats: null,
        currentProof: null,
        error: null,
        selectedNode: null,
        proofPath: [],
        selectedNodeDetails: null,
    }),
}));
