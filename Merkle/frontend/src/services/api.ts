import axios, { type AxiosInstance } from 'axios';
import type {
    Dataset,
    BuildTreeResponse,
    TreeStructure,
    ProofResponse,
    VerifyProofResponse,
    TreeStats,
    RootHistoryEntry,
    UpdateRecordResponse,
    DeleteRecordResponse,
    InsertRecordResponse,
    BatchUpdateResponse,
} from '../types/merkle.types';
import { API_CONFIG, SAMPLE_CONFIG } from '../config/constants';

class MerkleAPI {
    private client: AxiosInstance;

    constructor() {
        this.client = axios.create({
            baseURL: API_CONFIG.BASE_URL,
            headers: {
                'Content-Type': 'application/json',
            },
            timeout: API_CONFIG.TIMEOUT,
        });
    }

    // Health check
    async health(): Promise<{ status: string; service: string; version: string }> {
        console.log('[API] GET /health');
        try {
            const response = await this.client.get('/health', { timeout: API_CONFIG.HEALTH_CHECK_TIMEOUT });
            console.log('[API] Health response:', response.data);
            return response.data;
        } catch (error: any) {
            console.error('[API] Health check failed:', error);
            throw error;
        }
    }

    // Dataset operations
    async listDatasets(): Promise<Dataset[]> {
        console.log('[API] GET /datasets');
        try {
            const response = await this.client.get('/datasets', { timeout: API_CONFIG.DATASETS_TIMEOUT });
            console.log('[API] Datasets response:', response.data);
            const datasets = response.data.datasets || [];
            console.log('[API] Parsed datasets:', datasets.length, 'items');
            return datasets;
        } catch (error: any) {
            console.error('[API] Failed to list datasets:', error);
            throw error;
        }
    }

    async buildTree(datasetName: string): Promise<BuildTreeResponse> {
        console.log('[API] POST /tree/build with dataset:', datasetName);
        const response = await this.client.post('/tree/build', { dataset: datasetName });
        console.log('[API] Build tree response:', response.data);
        return response.data;
    }

    // Tree operations
    async getTreeStructure(maxDepth: number = 7): Promise<TreeStructure> {
        console.log('[API] GET /tree/structure with maxDepth:', maxDepth);
        const response = await this.client.get('/tree/structure', {
            params: { maxDepth },
        });
        console.log('[API] Tree structure response:', {
            success: response.data.success,
            nodes: response.data.nodes?.length || 0,
            depth: response.data.depth
        });
        return response.data;
    }

    async getRootHash(): Promise<{ success: boolean; rootHash: string; dataset: string }> {
        console.log('[API] GET /tree/root');
        const response = await this.client.get('/tree/root');
        console.log('[API] Root hash response:', response.data);
        return response.data;
    }

    async getNodeDetails(recordId: string): Promise<{
        success: boolean;
        record?: {
            id: string;
            rating: number;
            text: string;
            rawData: string;
        };
        error?: string;
    }> {
        const response = await this.client.post('/proof/generate', { recordId });
        return response.data;
    }

    // Proof operations
    async generateProof(recordId: string): Promise<ProofResponse> {
        const response = await this.client.post('/proof/generate', { recordId });
        return response.data;
    }

    // Optimized: Generate and verify proof in one call (server-side only, no data transfer)
    async generateAndVerifyProof(recordId: string): Promise<{
        success: boolean;
        recordId: string;
        dataIndex: number;
        isValid: boolean;
        proof: string[];
        record: {
            id: string;
            rating: number;
        };
        timings: {
            proofGeneration: number;
            verification: number;
            total: number;
        };
        error?: string;
    }> {
        const response = await this.client.post('/proof/generate-verify', { recordId });
        return response.data;
    }

    async verifyProof(proofData: {
        rawData: string;
        dataIndex: number;
        expectedRoot: string;
        proof: string[];
    }): Promise<VerifyProofResponse> {
        const response = await this.client.post('/proof/verify', proofData);
        return response.data;
    }

    // Statistics
    async getStats(): Promise<TreeStats> {
        console.log('[API] GET /stats');
        const response = await this.client.get('/stats');
        console.log('[API] Stats response:', {
            dataset: response.data.dataset,
            recordCount: response.data.recordCount,
            buildTime: response.data.buildTime
        });
        return response.data;
    }

    async getRootHistory(): Promise<RootHistoryEntry[]> {
        console.log('[API] GET /history');
        const response = await this.client.get('/history');
        console.log('[API] History response:', response.data.history?.length || 0, 'entries');
        return response.data.history || [];
    }

    // Record modification operations
    async updateRecord(recordId: string, newData: { text?: string; rating?: number }): Promise<UpdateRecordResponse> {
        console.log('[API] POST /record/update', { recordId, newData });
        const response = await this.client.post('/record/update', { recordId, newData });
        console.log('[API] Update record response:', response.data);
        return response.data;
    }

    async deleteRecord(recordId: string): Promise<DeleteRecordResponse> {
        console.log('[API] POST /record/delete', { recordId });
        const response = await this.client.post('/record/delete', { recordId });
        console.log('[API] Delete record response:', response.data);
        return response.data;
    }

    async insertRecord(recordData: { id?: string; text: string; rating: number }): Promise<InsertRecordResponse> {
        console.log('[API] POST /record/insert', recordData);
        const response = await this.client.post('/record/insert', { recordData });
        console.log('[API] Insert record response:', response.data);
        return response.data;
    }

    async batchUpdate(countOrUpdates: number | Array<{ recordId: string; newData: { text?: string; rating?: number } }>): Promise<BatchUpdateResponse> {
        // If a number is passed, generate random updates for that many records
        let updates;
        if (typeof countOrUpdates === 'number') {
            updates = Array.from({ length: countOrUpdates }, (_, i) => ({
                recordId: `${SAMPLE_CONFIG.ASIN}_${i}_0`,
                newData: {
                    text: `Modified review text ${i + 1}`,
                    rating: Math.random() * 5
                }
            }));
        } else {
            updates = countOrUpdates;
        }
        
        console.log('[API] POST /batch/update', { updates: updates.length });
        const response = await this.client.post('/batch/update', { updates });
        console.log('[API] Batch update response:', response.data);
        return response.data;
    }
}

export const api = new MerkleAPI();
export default api;
