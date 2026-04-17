// Merkle Tree Types
export interface TreeNode {
    index: number;
    depth: number;
    hash?: string;
    isLeaf: boolean;
    children?: TreeNode[];
}

export interface TreeStructure {
    totalNodes: number;
    leafCount: number;
    depth: number;
    nodes: TreeNode[];
}

export interface Dataset {
    name: string;
    path: string;
}

export interface Review {
    id: string;
    rating: number;
    text: string;
    rawData: string;
}

export interface Proof {
    recordId: string;
    dataIndex: number;
    proof: string[];
    record?: Review;
}

export interface TreeStats {
    dataset: string;
    recordCount: number;
    rootHash: string;
    buildTime: number;
    peakMemory: number;
    currentMemory: number;
    throughput?: number;
    avgHashTime?: number;
    avgProofLatency?: number;
}

export interface RootHistoryEntry {
    timestamp: string;
    dataset: string;
    records: number;
    root: string;
}

export interface BuildTreeResponse {
    success: boolean;
    dataset?: string;
    recordCount?: number;
    rootHash?: string;
    loadTime?: number;
    buildTime?: number;
    peakMemory?: number;
    error?: string;
}

export interface ProofResponse {
    success: boolean;
    recordId?: string;
    dataIndex?: number;
    proof?: string[];
    record?: Review;
    error?: string;
}

export interface VerifyProofResponse {
    success: boolean;
    isValid?: boolean;
    error?: string;
}

export interface ApiResponse<T> {
    success: boolean;
    data?: T;
    error?: string;
}

export interface UpdateRecordResponse {
    success: boolean;
    recordId?: string;
    dataIndex?: number;
    oldRoot?: string;
    newRoot?: string;
    updateTime?: number;
    rebuildTime?: number;
    recordCount?: number;
    error?: string;
}

export interface DeleteRecordResponse {
    success: boolean;
    recordId?: string;
    oldRoot?: string;
    newRoot?: string;
    rebuildTime?: number;
    recordCount?: number;
    error?: string;
}

export interface InsertRecordResponse {
    success: boolean;
    recordId?: string;
    oldRoot?: string;
    newRoot?: string;
    rebuildTime?: number;
    recordCount?: number;
    error?: string;
}

export interface BatchUpdateResponse {
    success: boolean;
    updatedCount?: number;
    oldRoot?: string;
    newRoot?: string;
    updateTime?: number;
    error?: string;
}
