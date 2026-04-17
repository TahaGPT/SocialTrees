import React from 'react';
import { X, Hash, Star, FileText, Database } from 'lucide-react';
import { motion, AnimatePresence } from 'framer-motion';
import { useMerkleStore } from '../../stores/merkleStore';

export const NodeDetailsModal: React.FC = () => {
    const { selectedNodeDetails } = useMerkleStore();

    if (!selectedNodeDetails) return null;

    const handleClose = () => {
        useMerkleStore.setState({ selectedNodeDetails: null });
    };

    const { nodeIndex, nodeData, isLeaf: explicitIsLeaf, noData } = selectedNodeDetails;
    // Use explicit isLeaf flag if provided, otherwise fall back to reviewId check
    const isLeaf = explicitIsLeaf !== undefined ? explicitIsLeaf : (nodeData?.reviewId !== undefined);
    const isRoot = nodeIndex === 0;

    return (
        <AnimatePresence>
            <div className="fixed inset-0 z-[300] flex items-center justify-center">
                {/* Backdrop */}
                <motion.div
                    initial={{ opacity: 0 }}
                    animate={{ opacity: 1 }}
                    exit={{ opacity: 0 }}
                    className="absolute inset-0 bg-black/80 backdrop-blur-sm"
                    onClick={handleClose}
                />

                {/* Modal */}
                <motion.div
                    initial={{ scale: 0.9, opacity: 0, y: 20 }}
                    animate={{ scale: 1, opacity: 1, y: 0 }}
                    exit={{ scale: 0.9, opacity: 0, y: 20 }}
                    className="relative w-full max-w-3xl max-h-[80vh] bg-black border-2 border-green-500 rounded-lg shadow-2xl overflow-hidden"
                >
                    {/* Header */}
                    <div className="flex items-center justify-between p-6 border-b border-green-500 bg-green-900/20">
                        <div className="flex items-center space-x-3">
                            <Database className="w-6 h-6 text-green-400" />
                            <h2 className="text-2xl font-bold text-green-400">
                                {isRoot ? 'Root Node' : isLeaf ? 'Leaf Node Details' : 'Internal Node'}
                            </h2>
                        </div>
                        <button
                            onClick={handleClose}
                            className="p-2 hover:bg-green-900/30 rounded-lg transition-colors"
                        >
                            <X className="w-6 h-6 text-green-500" />
                        </button>
                    </div>

                    {/* Content */}
                    <div className="p-6 overflow-y-auto max-h-[calc(80vh-100px)] custom-scrollbar">
                        {/* Node Info */}
                        <div className="space-y-4 mb-6">
                            <div className="grid grid-cols-2 gap-4">
                                <div className="bg-green-900/20 border border-green-500/30 rounded-lg p-4">
                                    <div className="flex items-center space-x-2 mb-2">
                                        <Hash className="w-4 h-4 text-green-400" />
                                        <span className="text-sm text-green-700">Node Index</span>
                                    </div>
                                    <p className="text-lg font-mono text-green-400">#{nodeIndex}</p>
                                </div>
                                <div className="bg-green-900/20 border border-green-500/30 rounded-lg p-4">
                                    <div className="flex items-center space-x-2 mb-2">
                                        <Database className="w-4 h-4 text-green-400" />
                                        <span className="text-sm text-green-700">Node Type</span>
                                    </div>
                                    <p className="text-lg font-semibold text-green-400">
                                        {isRoot ? 'Root' : isLeaf ? 'Leaf (Data)' : 'Internal'}
                                    </p>
                                </div>
                            </div>

                            {/* Hash */}
                            {nodeData?.hash && (
                                <div className="bg-green-900/20 border border-green-500/30 rounded-lg p-4">
                                    <div className="flex items-center space-x-2 mb-2">
                                        <Hash className="w-4 h-4 text-green-400" />
                                        <span className="text-sm text-green-700">Node Hash</span>
                                    </div>
                                    <p className="text-sm font-mono text-green-400 break-all">
                                        {nodeData.hash}
                                    </p>
                                </div>
                            )}

                            {/* Depth */}
                            {nodeData?.depth !== undefined && (
                                <div className="bg-green-900/20 border border-green-500/30 rounded-lg p-4">
                                    <div className="flex items-center space-x-2 mb-2">
                                        <Database className="w-4 h-4 text-green-400" />
                                        <span className="text-sm text-green-700">Tree Depth Level</span>
                                    </div>
                                    <p className="text-lg font-mono text-green-400">Level {nodeData.depth}</p>
                                </div>
                            )}
                        </div>

                        {/* Record Details (for leaf nodes) */}
                        {isLeaf && (
                            <>
                                <div className="border-t border-green-500/30 pt-6 mb-6">
                                    <h3 className="text-xl font-bold text-green-400 mb-4 flex items-center">
                                        <FileText className="w-5 h-5 mr-2" />
                                        Record Information
                                    </h3>

                                    <div className="space-y-4">
                                        {/* Record ID */}
                                        {nodeData.reviewId && (
                                            <div className="bg-green-900/20 border border-green-500/30 rounded-lg p-4">
                                                <div className="flex items-center space-x-2 mb-2">
                                                    <Hash className="w-4 h-4 text-green-400" />
                                                    <span className="text-sm text-green-700">Record ID</span>
                                                </div>
                                                <p className="text-lg font-mono text-green-400">{nodeData.reviewId}</p>
                                            </div>
                                        )}
                                        
                                        {/* Show message if no reviewId available */}
                                        {!nodeData.reviewId && (
                                            <div className="bg-yellow-900/20 border border-yellow-500/30 rounded-lg p-4">
                                                <p className="text-sm text-yellow-300">
                                                    {noData 
                                                        ? "This leaf node position doesn't contain data in the current tree structure. This can happen after deletions when nodes shift positions."
                                                        : "Record details are not available. The tree structure may need to be refreshed."}
                                                </p>
                                            </div>
                                        )}

                                        {/* Rating */}
                                        {nodeData.record?.rating !== undefined && (
                                            <div className="bg-green-900/20 border border-green-500/30 rounded-lg p-4">
                                                <div className="flex items-center space-x-2 mb-2">
                                                    <Star className="w-4 h-4 text-yellow-400" />
                                                    <span className="text-sm text-green-700">Rating</span>
                                                </div>
                                                <div className="flex items-center space-x-2">
                                                    <span className="text-2xl font-bold text-yellow-400">
                                                        {nodeData.record.rating}
                                                    </span>
                                                    <div className="flex">
                                                        {[...Array(5)].map((_, i) => (
                                                            <Star
                                                                key={i}
                                                                className={`w-5 h-5 ${
                                                                    i < nodeData.record.rating
                                                                        ? 'text-yellow-400 fill-yellow-400'
                                                                        : 'text-gray-600'
                                                                }`}
                                                            />
                                                        ))}
                                                    </div>
                                                </div>
                                            </div>
                                        )}

                                        {/* Review Text */}
                                        {nodeData.record?.text && (
                                            <div className="bg-green-900/20 border border-green-500/30 rounded-lg p-4">
                                                <div className="flex items-center space-x-2 mb-2">
                                                    <FileText className="w-4 h-4 text-green-400" />
                                                    <span className="text-sm text-green-700">Review Text</span>
                                                </div>
                                                <div className="max-h-60 overflow-y-auto custom-scrollbar">
                                                    <p className="text-sm text-green-300 whitespace-pre-wrap leading-relaxed">
                                                        {nodeData.record.text}
                                                    </p>
                                                </div>
                                            </div>
                                        )}

                                        {/* Raw Data */}
                                        {nodeData.record?.rawData && (
                                            <div className="bg-green-900/20 border border-green-500/30 rounded-lg p-4">
                                                <div className="flex items-center space-x-2 mb-2">
                                                    <Database className="w-4 h-4 text-green-400" />
                                                    <span className="text-sm text-green-700">Raw Data</span>
                                                </div>
                                                <div className="max-h-40 overflow-y-auto custom-scrollbar">
                                                    <p className="text-xs font-mono text-green-500 break-all">
                                                        {nodeData.record.rawData}
                                                    </p>
                                                </div>
                                            </div>
                                        )}
                                    </div>
                                </div>
                            </>
                        )}

                        {/* Internal/Root Node Info */}
                        {!isLeaf && (
                            <div className="bg-blue-900/20 border border-blue-500/30 rounded-lg p-4">
                                <p className="text-sm text-blue-300">
                                    {isRoot
                                        ? 'This is the root node of the Merkle tree. It represents the cryptographic commitment to all data in the tree.'
                                        : 'This is an internal node that combines hashes from its child nodes.'}
                                </p>
                            </div>
                        )}
                    </div>

                    {/* Footer */}
                    <div className="flex justify-end p-4 border-t border-green-500 bg-green-900/10">
                        <button
                            onClick={handleClose}
                            className="px-6 py-2 bg-green-600 hover:bg-green-700 text-white rounded-lg transition-colors font-medium"
                        >
                            Close
                        </button>
                    </div>
                </motion.div>
            </div>
        </AnimatePresence>
    );
};
