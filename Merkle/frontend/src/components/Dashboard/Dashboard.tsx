import React, { useEffect, useState } from 'react';
import { useMerkleStore } from '../../stores/merkleStore';
import api from '../../services/api';
import {
    Database,
    Hash,
    Clock,
    MemoryStick,
    CheckCircle2,
    AlertCircle,
    TrendingUp,
    Layers,
    Loader2
} from 'lucide-react';
import { formatNumber, formatTime, formatHash } from '../../lib/utils';

export const Dashboard: React.FC = () => {
    const { stats, rootHistory, currentDataset, treeStructure } = useMerkleStore();
    const [isLoadingStats, setIsLoadingStats] = useState(false);
    const [loadingMessage, setLoadingMessage] = useState<string>('');

    // Only auto-load stats if we have a tree structure (meaning a tree was actually built)
    useEffect(() => {
        // Don't auto-load if no tree structure exists - user must build tree first
        if (currentDataset && treeStructure) {
            console.log('[Dashboard] Auto-loading stats for dataset:', currentDataset);
            loadStats();
        } else {
            console.log('[Dashboard] Skipping auto-load - currentDataset:', currentDataset, 'treeStructure:', treeStructure ? 'exists' : 'null');
        }
    }, [currentDataset, treeStructure]);

    const loadStats = async () => {
        if (!currentDataset) {
            console.warn('[Dashboard] Cannot load stats - no dataset selected');
            return;
        }

        setIsLoadingStats(true);
        setLoadingMessage(`Loading statistics for ${currentDataset}...`);
        console.log('[Dashboard] Loading stats for dataset:', currentDataset);
        
        try {
            console.log('[Dashboard] Calling API: getStats()');
            const statsData = await api.getStats();
            console.log('[Dashboard] Stats received:', statsData);
            
            console.log('[Dashboard] Calling API: getRootHistory()');
            const historyData = await api.getRootHistory();
            console.log('[Dashboard] History received:', historyData);
            
            useMerkleStore.setState({ stats: statsData, rootHistory: historyData });
            setLoadingMessage('');
        } catch (error: any) {
            console.error('[Dashboard] Failed to load stats:', error);
            console.error('[Dashboard] Error details:', {
                message: error.message,
                response: error.response?.data,
                status: error.response?.status
            });
            setLoadingMessage(`Error loading stats: ${error.message || 'Unknown error'}`);
        } finally {
            setIsLoadingStats(false);
        }
    };

    if (isLoadingStats && loadingMessage) {
        return (
            <div className="glass-card p-12 text-center">
                <Loader2 className="w-16 h-16 mx-auto mb-4 text-primary-500 animate-spin" />
                <h3 className="text-xl font-semibold text-green-500 mb-2">
                    Loading Statistics...
                </h3>
                <p className="text-green-4000">
                    {loadingMessage}
                </p>
            </div>
        );
    }

    if (!stats) {
        return (
            <div className="glass-card p-12 text-center">
                <Database className="w-16 h-16 mx-auto mb-4 text-green-800" />
                <h3 className="text-xl font-semibold text-green-500 mb-2">
                    No Tree Loaded
                </h3>
                <p className="text-green-4000">
                    Select a dataset and build a Merkle Tree to view statistics
                </p>
                {currentDataset && !treeStructure && (
                    <p className="text-warning-500 mt-2 text-sm">
                        Dataset "{currentDataset}" selected but tree not built yet. Click "Build Tree" to proceed.
                    </p>
                )}
            </div>
        );
    }

    const latestHistory = rootHistory.filter(h => h.dataset === stats.dataset)[0];
    const isRootMatch = latestHistory && latestHistory.root === stats.rootHash;

    return (
        <div className="space-y-6 animate-fade-in">
            {/* Header */}
            <div className="flex items-center justify-between">
                <div>
                    <h2 className="text-3xl font-bold text-green-400">Dashboard</h2>
                    <p className="text-green-600 mt-1">
                        Real-time metrics and integrity status
                    </p>
                </div>
                <button
                    onClick={loadStats}
                    disabled={isLoadingStats}
                    className="btn-secondary"
                >
                    {isLoadingStats ? 'Refreshing...' : 'Refresh Stats'}
                </button>
            </div>

            {/* Metrics Grid */}
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
                {/* Record Count */}
                <div className="metric-card">
                    <div className="flex items-center justify-between mb-4">
                        <Database className="w-8 h-8 text-primary-500" />
                        <span className="text-xs font-medium text-green-600 uppercase tracking-wide">
                            Records
                        </span>
                    </div>
                    <div className="text-3xl font-bold text-green-400 mb-1">
                        {formatNumber(stats.recordCount)}
                    </div>
                    <div className="text-sm text-green-600">
                        Dataset: {stats.dataset}
                    </div>
                </div>

                {/* Build Time */}
                <div className="metric-card">
                    <div className="flex items-center justify-between mb-4">
                        <Clock className="w-8 h-8 text-success-500" />
                        <span className="text-xs font-medium text-green-600 uppercase tracking-wide">
                            Build Time
                        </span>
                    </div>
                    <div className="text-3xl font-bold text-green-400 mb-1">
                        {formatTime(stats.buildTime * 1000)}
                    </div>
                    <div className="text-sm text-green-600">
                        {(stats.recordCount / stats.buildTime).toFixed(0)} records/sec
                    </div>
                </div>

                {/* Memory Usage */}
                <div className="metric-card">
                    <div className="flex items-center justify-between mb-4">
                        <MemoryStick className="w-8 h-8 text-warning-500" />
                        <span className="text-xs font-medium text-green-600 uppercase tracking-wide">
                            Peak Memory
                        </span>
                    </div>
                    <div className="text-3xl font-bold text-green-400 mb-1">
                        {stats.peakMemory.toFixed(0)} MB
                    </div>
                    <div className="text-sm text-green-600">
                        Current: {stats.currentMemory.toFixed(0)} MB
                    </div>
                </div>

                {/* Integrity Status */}
                <div className="metric-card">
                    <div className="flex items-center justify-between mb-4">
                        {isRootMatch ? (
                            <CheckCircle2 className="w-8 h-8 text-success-500" />
                        ) : (
                            <AlertCircle className="w-8 h-8 text-warning-500" />
                        )}
                        <span className="text-xs font-medium text-green-600 uppercase tracking-wide">
                            Integrity
                        </span>
                    </div>
                    <div className={`text-2xl font-bold mb-1 ${isRootMatch ? 'text-success-500' : 'text-warning-500'}`}>
                        {isRootMatch ? 'VERIFIED' : 'CHANGED'}
                    </div>
                    <div className="text-sm text-green-600">
                        {isRootMatch ? 'Matches history' : 'Root hash differs'}
                    </div>
                </div>
            </div>

            {/* Root Hash Card */}
            <div className="glass-card p-6">
                <div className="flex items-center space-x-3 mb-4">
                    <Hash className="w-6 h-6 text-gold-500" />
                    <h3 className="text-xl font-semibold text-green-400">Current Root Hash</h3>
                </div>
                <div className="hash-display text-base">
                    {stats.rootHash}
                </div>
                {latestHistory && (
                    <div className="mt-4 pt-4 border-t border-green-500/30">
                        <div className="flex items-center justify-between text-sm">
                            <span className="text-green-600">Historical Root:</span>
                            <span className={`font-mono ${isRootMatch ? 'text-success-500' : 'text-warning-500'}`}>
                                {formatHash(latestHistory.root, 20)}
                            </span>
                        </div>
                        <div className="flex items-center justify-between text-sm mt-2">
                            <span className="text-green-600">Timestamp:</span>
                            <span className="text-green-500">{latestHistory.timestamp}</span>
                        </div>
                    </div>
                )}
            </div>

            {/* Performance Insights */}
            <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
                {/* Tree Statistics */}
                <div className="glass-card p-6">
                    <div className="flex items-center space-x-3 mb-4">
                        <Layers className="w-6 h-6 text-primary-500" />
                        <h3 className="text-xl font-semibold text-green-400">Tree Statistics</h3>
                    </div>
                    <div className="space-y-3">
                        <div className="flex justify-between items-center">
                            <span className="text-green-600">Tree Type:</span>
                            <span className="text-green-300 font-medium">Quad-Tree (4-ary)</span>
                        </div>
                        <div className="flex justify-between items-center">
                            <span className="text-green-600">Estimated Depth:</span>
                            <span className="text-green-300 font-medium">
                                {Math.ceil(Math.log(stats.recordCount) / Math.log(4))} levels
                            </span>
                        </div>
                        <div className="flex justify-between items-center">
                            <span className="text-green-600">Hash Algorithm:</span>
                            <span className="text-green-300 font-medium">SHA-256</span>
                        </div>
                        <div className="flex justify-between items-center">
                            <span className="text-green-600">Avg Hash Time:</span>
                            <span className="text-green-300 font-medium">
                                {((stats.buildTime * 1000) / stats.recordCount).toFixed(4)} ms
                            </span>
                        </div>
                    </div>
                </div>

                {/* Performance Metrics */}
                <div className="glass-card p-6">
                    <div className="flex items-center space-x-3 mb-4">
                        <TrendingUp className="w-6 h-6 text-success-500" />
                        <h3 className="text-xl font-semibold text-green-400">Performance</h3>
                    </div>
                    <div className="space-y-3">
                        <div className="flex justify-between items-center">
                            <span className="text-green-600">Throughput:</span>
                            <span className="text-success-500 font-medium">
                                {stats.throughput 
                                    ? formatNumber(Math.floor(stats.throughput))
                                    : formatNumber(Math.floor(stats.recordCount / stats.buildTime))} rec/s
                            </span>
                        </div>
                        <div className="flex justify-between items-center">
                            <span className="text-green-600">Avg Hash Time:</span>
                            <span className="text-green-300 font-medium">
                                {stats.avgHashTime 
                                    ? stats.avgHashTime.toFixed(4)
                                    : ((stats.buildTime * 1000) / stats.recordCount).toFixed(4)} ms
                            </span>
                        </div>
                        {stats.avgProofLatency !== undefined && (
                            <>
                                <div className="flex justify-between items-center">
                                    <span className="text-green-600">Avg Proof Latency:</span>
                                    <span className={`font-bold ${stats.avgProofLatency < 100 ? 'text-success-500' : 'text-warning-500'}`}>
                                        {stats.avgProofLatency.toFixed(3)} ms
                                    </span>
                                </div>
                                <div className="flex justify-between items-center">
                                    <span className="text-green-600">Target Status:</span>
                                    <span className={`font-bold ${stats.avgProofLatency < 100 ? 'text-success-500' : 'text-warning-500'}`}>
                                        {stats.avgProofLatency < 100 ? '✓ Under 100ms' : '✗ Over 100ms'}
                                    </span>
                                </div>
                                <div className="mt-2 pt-2 border-t border-green-500/30">
                                    <div className="text-xs text-green-600 text-center">
                                        {stats.avgProofLatency < 100 
                                            ? `${Math.floor(100 / stats.avgProofLatency)}x faster than target!`
                                            : 'Optimization needed'}
                                    </div>
                                </div>
                            </>
                        )}
                        <div className="flex justify-between items-center">
                            <span className="text-green-600">Memory Efficiency:</span>
                            <span className="text-success-500 font-medium">
                                {((stats.peakMemory * 1024 * 1024) / stats.recordCount).toFixed(0)} bytes/record
                            </span>
                        </div>
                        <div className="flex justify-between items-center">
                            <span className="text-green-600">Build Speed:</span>
                            <span className="text-success-500 font-medium">
                                {stats.buildTime < 10 ? 'Excellent' : stats.buildTime < 30 ? 'Good' : 'Moderate'}
                            </span>
                        </div>
                        <div className="flex justify-between items-center">
                            <span className="text-green-600">Optimization:</span>
                            <span className="text-success-500 font-medium">
                                Parallel + MMAP
                            </span>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
};
