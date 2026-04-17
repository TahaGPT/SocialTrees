import React, { useState } from 'react';
import { useMerkleStore } from '../../stores/merkleStore';
import api from '../../services/api';
import {
    Shield,
    FileCheck2,
    Edit3,
    Layers,
    Trash2,
    PlusCircle,
    Zap,
    Database,
    BarChart3,
    History,
    Loader2,
    X,
    Github
} from 'lucide-react';
import { motion, AnimatePresence } from 'framer-motion';
import { TREE_CONFIG, PERFORMANCE_TARGETS, SAMPLE_CONFIG } from '../../config/constants';

type OperationType =
    | 'verify'
    | 'proof'
    | 'modify'
    | 'batch'
    | 'delete'
    | 'insert'
    | 'benchmark'
    | 'sample'
    | 'stats'
    | 'history'
    | null;

export const OperationsSidebar: React.FC = () => {
    const { stats, currentDataset } = useMerkleStore();
    const [activeOperation, setActiveOperation] = useState<OperationType>(null);
    const [isLoading, setIsLoading] = useState(false);
    const [result, setResult] = useState<any>(null);

    // Form states
    const [recordId, setRecordId] = useState('');
    const [newText, setNewText] = useState('');
    const [newRating, setNewRating] = useState('5.0');
    const [batchCount, setBatchCount] = useState('5');

    const operations = [
        { id: 'verify', label: '1. Verify Integrity', icon: Shield, color: 'green' },
        { id: 'proof', label: '2. Generate Proof', icon: FileCheck2, color: 'cyan' },
        { id: 'modify', label: '3. Modify Record', icon: Edit3, color: 'yellow' },
        { id: 'batch', label: '4. Batch Modify', icon: Layers, color: 'yellow' },
        { id: 'delete', label: '5. Delete Record', icon: Trash2, color: 'red' },
        { id: 'insert', label: '6. Insert Record', icon: PlusCircle, color: 'green' },
        { id: 'benchmark', label: '7. Benchmark Test', icon: Zap, color: 'yellow' },
        { id: 'sample', label: '8. Dataset Sample', icon: Database, color: 'cyan' },
        { id: 'stats', label: '9. Performance Stats', icon: BarChart3, color: 'green' },
        { id: 'history', label: '10. Root History', icon: History, color: 'cyan' },
    ];

    const handleOperation = async (operation: OperationType) => {
        if (operation !== 'verify' && operation !== 'stats' && operation !== 'history' && 
            operation !== 'sample' && operation !== 'benchmark') {
            // For operations that need input, just open the modal
            setActiveOperation(operation);
            setResult(null);
            return;
        }

        // For operations that don't need input, execute immediately
        setActiveOperation(operation);
        setResult(null);
        await executeOperation(operation);
    };

    const executeOperation = async (operation: OperationType) => {
        setIsLoading(true);
        const startTime = performance.now();

        try {
            switch (operation) {
                case 'verify':
                    const rootResponse = await api.getRootHash();
                    const verifyTime = performance.now() - startTime;
                    setResult({
                        type: 'success',
                        message: 'Integrity Verified ✓',
                        time: verifyTime,
                        data: rootResponse,
                    });
                    break;

                case 'proof':
                    if (!recordId) {
                        setResult({ type: 'error', message: 'Please enter a record ID' });
                        setIsLoading(false);
                        return;
                    }
                    
                    // Use optimized server-side proof generation + verification
                    const proofStart = performance.now();
                    const optimizedResponse = await api.generateAndVerifyProof(recordId);
                    const clientTime = performance.now() - proofStart;
                    
                    if (optimizedResponse.success) {
                        // Calculate proof path for visualization
                        const dataIndex = optimizedResponse.dataIndex;
                        const recordCount = stats?.recordCount || TREE_CONFIG.DEFAULT_RECORD_COUNT;
                        let leafCapacity = 1;
                        while (leafCapacity < recordCount) {
                            leafCapacity *= TREE_CONFIG.ARITY;
                        }
                        const leafStartIdx = Math.floor((leafCapacity - 1) / (TREE_CONFIG.ARITY - 1));
                        const proofPath = [leafStartIdx + dataIndex];
                        
                        let nodeIndex = leafStartIdx + dataIndex;
                        while (nodeIndex > 0) {
                            const parentIndex = Math.floor((nodeIndex - 1) / TREE_CONFIG.ARITY);
                            proofPath.push(parentIndex);
                            nodeIndex = parentIndex;
                        }
                        
                        // Update store with proof path for visualization
                        useMerkleStore.setState({ 
                            proofPath: proofPath,
                            selectedNode: leafStartIdx + dataIndex
                        });
                        
                        const serverTime = optimizedResponse.timings.total;
                        const networkOverhead = clientTime - serverTime;
                        const meetsTarget = serverTime < PERFORMANCE_TARGETS.PROOF_GENERATION_MAX_MS;
                        
                        setResult({
                            type: 'success',
                            message: `Proof Generated & Verified ${optimizedResponse.isValid ? '✓' : '✗'}`,
                            time: clientTime,
                            data: {
                                ...optimizedResponse,
                                proofPath: proofPath,
                                timings: {
                                    serverTime: `${serverTime.toFixed(2)} ms`,
                                    networkOverhead: `${networkOverhead.toFixed(2)} ms`,
                                    clientTotal: `${clientTime.toFixed(2)} ms`,
                                    meetsTarget: meetsTarget ? '✓ < 100ms' : '✗ > 100ms'
                                }
                            },
                        });
                    } else {
                        setResult({
                            type: 'error',
                            message: optimizedResponse.error || 'Failed to generate proof',
                        });
                    }
                    break;

                case 'modify':
                    if (!recordId) {
                        setResult({ type: 'error', message: 'Please enter a record ID' });
                        setIsLoading(false);
                        return;
                    }
                    const modifyStart = performance.now();
                    const updateResponse = await api.updateRecord(recordId, {
                        text: newText || undefined,
                        rating: newRating ? parseFloat(newRating) : undefined,
                    });
                    const modifyTime = performance.now() - modifyStart;
                    
                    setResult({
                        type: updateResponse.success ? 'success' : 'error',
                        message: updateResponse.success
                            ? `Record Updated! Time: ${modifyTime.toFixed(2)} ms`
                            : updateResponse.error,
                        time: modifyTime,
                        data: updateResponse,
                    });
                    
                    if (updateResponse.success) {
                        const treeResponse = await api.getTreeStructure();
                        useMerkleStore.setState({ treeStructure: treeResponse });
                    }
                    break;

                case 'batch':
                    const count = parseInt(batchCount) || 5;
                    const batchStart = performance.now();
                    const batchResponse = await api.batchUpdate(count);
                    const batchTime = performance.now() - batchStart;
                    
                    setResult({
                        type: batchResponse.success ? 'success' : 'error',
                        message: batchResponse.success
                            ? `Batch Modified ${count} records in ${batchTime.toFixed(2)} ms`
                            : batchResponse.error,
                        time: batchTime,
                        data: batchResponse,
                    });
                    
                    if (batchResponse.success) {
                        const treeResponse = await api.getTreeStructure();
                        const statsResponse = await api.getStats();
                        useMerkleStore.setState({ 
                            treeStructure: treeResponse,
                            stats: statsResponse
                        });
                    }
                    break;

                case 'delete':
                    if (!recordId) {
                        setResult({ type: 'error', message: 'Please enter a record ID' });
                        setIsLoading(false);
                        return;
                    }
                    const deleteStart = performance.now();
                    const deleteResponse = await api.deleteRecord(recordId);
                    const deleteTime = performance.now() - deleteStart;
                    
                    setResult({
                        type: deleteResponse.success ? 'success' : 'error',
                        message: deleteResponse.success
                            ? `Deleted in ${deleteTime.toFixed(2)} ms! Records now: ${deleteResponse.recordCount}`
                            : deleteResponse.error,
                        time: deleteTime,
                        data: deleteResponse,
                    });
                    
                    if (deleteResponse.success) {
                        const treeResponse = await api.getTreeStructure();
                        const statsResponse = await api.getStats();
                        useMerkleStore.setState({ 
                            treeStructure: treeResponse,
                            stats: statsResponse
                        });
                    }
                    break;

                case 'insert':
                    const insertStart = performance.now();
                    const insertResponse = await api.insertRecord({
                        text: newText || SAMPLE_CONFIG.DEFAULT_TEXT,
                        rating: newRating ? parseFloat(newRating) : SAMPLE_CONFIG.DEFAULT_RATING,
                    });
                    const insertTime = performance.now() - insertStart;
                    
                    setResult({
                        type: insertResponse.success ? 'success' : 'error',
                        message: insertResponse.success
                            ? `Inserted in ${insertTime.toFixed(2)} ms! ID: ${insertResponse.recordId}`
                            : insertResponse.error,
                        time: insertTime,
                        data: insertResponse,
                    });
                    
                    if (insertResponse.success) {
                        const treeResponse = await api.getTreeStructure();
                        const statsResponse = await api.getStats();
                        useMerkleStore.setState({ 
                            treeStructure: treeResponse,
                            stats: statsResponse
                        });
                    }
                    break;

                case 'stats':
                    const statsResponse = await api.getStats();
                    const statsTime = performance.now() - startTime;
                    setResult({
                        type: 'success',
                        message: 'Performance Statistics',
                        time: statsTime,
                        data: statsResponse,
                    });
                    break;

                case 'history':
                    const historyResponse = await api.getRootHistory();
                    const historyTime = performance.now() - startTime;
                    setResult({
                        type: 'success',
                        message: 'Root Hash History',
                        time: historyTime,
                        data: historyResponse,
                    });
                    break;

                case 'benchmark':
                    const benchmarkStart = performance.now();
                    const proofTimes: number[] = [];
                    const sampleSize = Math.min(100, stats?.recordCount || 10);
                    
                    for (let i = 0; i < sampleSize; i++) {
                        const idx = Math.floor((i * (stats?.recordCount || 40)) / sampleSize);
                        const proofStart = performance.now();
                        try {
                            await api.generateAndVerifyProof(`${SAMPLE_CONFIG.ASIN}_${idx}_0`);
                            proofTimes.push(performance.now() - proofStart);
                        } catch (e) {
                            // Skip failed proofs
                        }
                    }
                    
                    const benchmarkTime = performance.now() - benchmarkStart;
                    const avgProofTime = proofTimes.length > 0 ? proofTimes.reduce((a, b) => a + b, 0) / proofTimes.length : 0;
                    const minProofTime = proofTimes.length > 0 ? Math.min(...proofTimes) : 0;
                    const maxProofTime = proofTimes.length > 0 ? Math.max(...proofTimes) : 0;
                    
                    setResult({
                        type: 'success',
                        message: `Benchmark Complete: ${sampleSize} Proofs`,
                        time: benchmarkTime,
                        data: {
                            totalProofs: sampleSize,
                            successfulProofs: proofTimes.length,
                            totalTime: `${benchmarkTime.toFixed(2)} ms`,
                            avgProofTime: `${avgProofTime.toFixed(3)} ms`,
                            minProofTime: `${minProofTime.toFixed(3)} ms`,
                            maxProofTime: `${maxProofTime.toFixed(3)} ms`,
                            throughput: `${((proofTimes.length / benchmarkTime) * 1000).toFixed(0)} proofs/s`,
                        },
                    });
                    break;

                case 'sample':
                    const sampleStatsResponse = await api.getStats();
                    const sampleTime = performance.now() - startTime;
                    
                    const sampleData = {
                        dataset: sampleStatsResponse.dataset,
                        recordCount: sampleStatsResponse.recordCount,
                        sampleSize: Math.min(5, sampleStatsResponse.recordCount),
                        sampleRecords: Array.from({length: Math.min(5, sampleStatsResponse.recordCount)}, (_, i) => ({
                            index: i,
                            recordId: `${SAMPLE_CONFIG.ASIN}_${i}_0`,
                            estimated: true
                        }))
                    };
                    
                    setResult({
                        type: 'success',
                        message: 'Dataset Sample (Top 5 Records)',
                        time: sampleTime,
                        data: sampleData,
                    });
                    break;
            }
        } catch (error: any) {
            setResult({
                type: 'error',
                message: error.message || 'Operation failed',
            });
        } finally {
            setIsLoading(false);
        }
    };

    const closeOperation = () => {
        setActiveOperation(null);
        setResult(null);
        setRecordId('');
        setNewText('');
        setNewRating('5.0');
    };

    return (
        <div className="fixed left-0 top-0 h-screen w-80 bg-black border-r-2 border-green-500 overflow-y-auto custom-scrollbar z-[100]">
            {/* Header */}
            <div className="p-6 border-b border-green-500">
                <h2 className="text-2xl font-bold text-green-500 mb-2">Merkle Tree: Integrity Verification System</h2>
                <p className="text-green-700 text-sm">
                    {currentDataset || 'No dataset loaded'}
                </p>
            </div>

            {/* Operations List */}
            <div className="p-4 space-y-2">
                {operations.map((op) => {
                    const Icon = op.icon;
                    return (
                        <button
                            key={op.id}
                            onClick={() => handleOperation(op.id as OperationType)}
                            disabled={!stats && op.id !== 'stats'}
                            className={`w-full flex items-center space-x-3 p-3 rounded-lg border transition-all ${
                                activeOperation === op.id
                                    ? 'bg-green-900 border-green-500 text-green-400'
                                    : 'bg-black border-green-500/30 text-green-500 hover:bg-green-900/30 hover:border-green-500'
                            } disabled:opacity-50 disabled:cursor-not-allowed`}
                        >
                            <Icon className="w-5 h-5" />
                            <span className="text-sm font-medium">{op.label}</span>
                        </button>
                    );
                })}
            </div>

            {/* GitHub Link */}
            <div className="p-4 border-t border-green-500/30 mt-24">
                <a
                    href="https://github.com/Z-zaratahir/Linear-Quad-Merkle-Tree"
                    target="_blank"
                    rel="noopener noreferrer"
                    className="w-full flex items-center justify-center space-x-3 p-3 rounded-lg border border-green-500/30 bg-black text-green-500 hover:bg-green-900/30 hover:border-green-500 transition-all"
                >
                    <Github className="w-5 h-5" />
                    <span className="text-sm font-medium">View on GitHub</span>
                </a>
            </div>

            {/* Operation Panel */}
            <AnimatePresence>
                {activeOperation && (
                    <motion.div
                        initial={{ opacity: 0, y: 20 }}
                        animate={{ opacity: 1, y: 0 }}
                        exit={{ opacity: 0, y: 20 }}
                        className="fixed inset-0 bg-black/90 flex items-center justify-center z-[200]"
                        onClick={closeOperation}
                    >
                        <div
                            className="bg-black border-2 border-green-500 rounded-lg p-6 max-w-lg w-full mx-4 max-h-[80vh] overflow-y-auto"
                            onClick={(e) => e.stopPropagation()}
                        >
                            <div className="flex justify-between items-center mb-4">
                                <h3 className="text-xl font-bold text-green-500">
                                    {operations.find((o) => o.id === activeOperation)?.label}
                                </h3>
                                <button
                                    onClick={closeOperation}
                                    className="text-green-500 hover:text-green-400"
                                >
                                    <X className="w-6 h-6" />
                                </button>
                            </div>

                            {/* Input Forms */}
                            {(activeOperation === 'modify' ||
                                activeOperation === 'delete' ||
                                activeOperation === 'proof') && (
                                <div className={`mb-4 ${activeOperation === 'proof' ? 'flex items-center gap-2' : 'space-y-3'}`}>
                                    <input
                                        type="text"
                                        value={recordId}
                                        onChange={(e) => setRecordId(e.target.value)}
                                        placeholder={`Record ID (e.g., ${SAMPLE_CONFIG.ASIN}_0_0)`}
                                        className="input flex-1"
                                    />
                                    {activeOperation === 'proof' && (
                                        <button
                                            onClick={() => executeOperation('proof')}
                                            disabled={isLoading || !recordId}
                                            className="btn-primary px-6 py-3 whitespace-nowrap h-12"
                                        >
                                            Generate
                                        </button>
                                    )}
                                </div>
                            )}

                            {(activeOperation === 'modify' || activeOperation === 'insert') && (
                                <div className="space-y-3 mb-4">
                                    <input
                                        type="text"
                                        value={newText}
                                        onChange={(e) => setNewText(e.target.value)}
                                        placeholder="New review text"
                                        className="input"
                                    />
                                    <input
                                        type="number"
                                        value={newRating}
                                        onChange={(e) => setNewRating(e.target.value)}
                                        placeholder="Rating (0-5)"
                                        min="0"
                                        max="5"
                                        step="0.1"
                                        className="input"
                                    />
                                </div>
                            )}

                            {activeOperation === 'batch' && (
                                <div className="space-y-3 mb-4">
                                    <input
                                        type="number"
                                        value={batchCount}
                                        onChange={(e) => setBatchCount(e.target.value)}
                                        placeholder="Number of records to modify"
                                        min="1"
                                        max="100"
                                        className="input"
                                    />
                                </div>
                            )}

                            {/* Loading State */}
                            {isLoading && (
                                <div className="flex items-center justify-center py-8">
                                    <Loader2 className="w-8 h-8 text-green-500 animate-spin" />
                                    <span className="ml-3 text-green-500">Processing...</span>
                                </div>
                            )}

                            {/* Result Display */}
                            {result && !isLoading && (
                                <div
                                    className={`p-4 rounded-lg border mb-4 ${
                                        result.type === 'success'
                                            ? 'bg-green-900/20 border-green-500 text-green-400'
                                            : 'bg-red-900/20 border-red-500 text-red-400'
                                    }`}
                                >
                                    <p className="font-semibold mb-2">{result.message}</p>
                                    
                                    {/* Timing Information */}
                                    {result.time !== undefined && (
                                        <div className="mt-2 p-2 bg-black/50 rounded border border-green-500/30">
                                            <p className="text-xs font-mono text-green-300">
                                                ⚡ Total Time: <span className="text-yellow-400 font-bold">{result.time.toFixed(2)} ms</span>
                                                {result.time < PERFORMANCE_TARGETS.PROOF_GENERATION_MAX_MS && <span className="text-green-400 ml-2">✓ &lt;{PERFORMANCE_TARGETS.PROOF_GENERATION_MAX_MS}ms target met!</span>}
                                            </p>
                                            {result.proofTime !== undefined && (
                                                <>
                                                    <p className="text-xs font-mono text-green-300 mt-1">
                                                        📊 Proof Generation: <span className="text-cyan-400">{result.proofTime.toFixed(2)} ms</span>
                                                    </p>
                                                    <p className="text-xs font-mono text-green-300">
                                                        ✓ Verification: <span className="text-cyan-400">{result.verifyTime?.toFixed(2)} ms</span>
                                                    </p>
                                                </>
                                            )}
                                        </div>
                                    )}
                                    
                                    {result.data && (
                                        <pre className="text-xs overflow-x-auto mt-2 p-2 bg-black rounded border border-green-500/30 max-h-64">
                                            {JSON.stringify(result.data, null, 2)}
                                        </pre>
                                    )}
                                </div>
                            )}

                            {/* Action Button */}
                            {!isLoading && !result && activeOperation !== 'proof' && (
                                <button
                                    onClick={() => executeOperation(activeOperation)}
                                    className="btn-primary w-full"
                                >
                                    Execute Operation
                                </button>
                            )}
                            
                            {/* Close Button after result */}
                            {!isLoading && result && (
                                <button
                                    onClick={closeOperation}
                                    className="btn-secondary w-full"
                                >
                                    Close
                                </button>
                            )}
                        </div>
                    </motion.div>
                )}
            </AnimatePresence>
        </div>
    );
};
