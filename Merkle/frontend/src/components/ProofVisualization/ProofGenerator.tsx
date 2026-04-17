import React, { useState } from 'react';
import { useMerkleStore } from '../../stores/merkleStore';
import api from '../../services/api';
import { Play, CheckCircle2, XCircle, Loader2, Hash } from 'lucide-react';
import { motion, AnimatePresence } from 'framer-motion';
import { formatHash } from '../../lib/utils';

export const ProofGenerator: React.FC = () => {
    const { currentProof, stats } = useMerkleStore();
    const [recordId, setRecordId] = useState('');
    const [isGenerating, setIsGenerating] = useState(false);
    const [isVerifying, setIsVerifying] = useState(false);
    const [verificationResult, setVerificationResult] = useState<boolean | null>(null);
    const [currentStep, setCurrentStep] = useState(0);

    const handleGenerateProof = async () => {
        if (!recordId.trim()) return;

        setIsGenerating(true);
        setVerificationResult(null);
        setCurrentStep(0);

        try {
            const response = await api.generateProof(recordId);

            if (response.success && response.proof) {
                // Calculate proof path (nodes traversed from leaf to root)
                const proofPath: number[] = [];
                let currentIndex = response.dataIndex!;
                
                // Add leaf node
                const leafStartIdx = Math.floor((Math.pow(4, Math.ceil(Math.log(stats?.recordCount || 40) / Math.log(4))) - 1) / 3);
                proofPath.push(leafStartIdx + currentIndex);
                
                // Traverse to root
                let nodeIndex = leafStartIdx + currentIndex;
                while (nodeIndex > 0) {
                    const parentIndex = Math.floor((nodeIndex - 1) / 4);
                    proofPath.push(parentIndex);
                    nodeIndex = parentIndex;
                }
                
                useMerkleStore.setState({
                    currentProof: {
                        recordId: response.recordId!,
                        dataIndex: response.dataIndex!,
                        proof: response.proof,
                        record: response.record,
                    },
                    proofPath: proofPath,
                });
            } else {
                useMerkleStore.setState({ error: response.error || 'Failed to generate proof' });
            }
        } catch (error) {
            useMerkleStore.setState({ error: 'Failed to generate proof' });
        } finally {
            setIsGenerating(false);
        }
    };

    const handleVerifyProof = async () => {
        if (!currentProof || !stats) return;

        setIsVerifying(true);
        setCurrentStep(0);

        try {
            // Animate through proof steps
            for (let i = 0; i <= currentProof.proof.length; i++) {
                setCurrentStep(i);
                await new Promise(resolve => setTimeout(resolve, 800));
            }

            const response = await api.verifyProof({
                rawData: currentProof.record?.rawData || '',
                dataIndex: currentProof.dataIndex,
                expectedRoot: stats.rootHash,
                proof: currentProof.proof,
            });

            setVerificationResult(response.isValid || false);
        } catch (error) {
            setVerificationResult(false);
        } finally {
            setIsVerifying(false);
        }
    };

    return (
        <div className="space-y-6">
            {/* Input Section */}
            <div className="glass-card p-6">
                <h3 className="text-xl font-semibold text-green-400 mb-4">Generate Proof</h3>
                <div className="flex space-x-3">
                    <input
                        type="text"
                        value={recordId}
                        onChange={(e) => setRecordId(e.target.value)}
                        placeholder="Enter Record ID (e.g., B001234_0_42)"
                        className="input flex-1"
                        disabled={isGenerating}
                    />
                    <button
                        onClick={handleGenerateProof}
                        disabled={isGenerating || !recordId.trim()}
                        className="btn-primary px-6"
                    >
                        {isGenerating ? (
                            <>
                                <Loader2 className="w-5 h-5 mr-2 animate-spin" />
                                Generating...
                            </>
                        ) : (
                            <>
                                <Play className="w-5 h-5 mr-2" />
                                Generate
                            </>
                        )}
                    </button>
                </div>
            </div>

            {/* Proof Display */}
            <AnimatePresence>
                {currentProof && (
                    <motion.div
                        initial={{ opacity: 0, y: 20 }}
                        animate={{ opacity: 1, y: 0 }}
                        exit={{ opacity: 0, y: -20 }}
                        className="glass-card p-6 space-y-6"
                    >
                        {/* Record Info */}
                        <div>
                            <h3 className="text-xl font-semibold text-green-400 mb-4">Proof Details</h3>
                            <div className="space-y-3">
                                <div className="flex justify-between items-center">
                                    <span className="text-green-600">Record ID:</span>
                                    <span className="font-mono text-primary-400">{currentProof.recordId}</span>
                                </div>
                                <div className="flex justify-between items-center">
                                    <span className="text-green-600">Data Index:</span>
                                    <span className="font-mono text-green-300">{currentProof.dataIndex}</span>
                                </div>
                                {currentProof.record && (
                                    <>
                                        <div className="flex justify-between items-center">
                                            <span className="text-green-600">Rating:</span>
                                            <span className="text-green-300">{currentProof.record.rating} ⭐</span>
                                        </div>
                                        <div>
                                            <span className="text-green-600 block mb-2">Review Text:</span>
                                            <p className="text-green-300 text-sm bg-black p-3 rounded border border-green-500/30">
                                                {currentProof.record.text.substring(0, 200)}
                                                {currentProof.record.text.length > 200 && '...'}
                                            </p>
                                        </div>
                                    </>
                                )}
                            </div>
                        </div>

                        {/* Proof Path */}
                        <div>
                            <h4 className="text-lg font-semibold text-green-400 mb-3">
                                Proof Path ({currentProof.proof.length} hashes)
                            </h4>
                            <div className="space-y-2 max-h-64 overflow-y-auto custom-scrollbar">
                                {currentProof.proof.map((hash, index) => (
                                    <motion.div
                                        key={index}
                                        initial={{ opacity: 0, x: -20 }}
                                        animate={{
                                            opacity: isVerifying && index <= currentStep ? 1 : 0.5,
                                            x: 0,
                                            scale: isVerifying && index === currentStep ? 1.02 : 1,
                                        }}
                                        transition={{ delay: index * 0.1 }}
                                        className={`hash-display ${isVerifying && index === currentStep ? 'glow-primary' : ''}`}
                                    >
                                        <div className="flex items-center justify-between">
                                            <span className="text-green-4000 mr-3">#{index + 1}</span>
                                            <span className="flex-1">{formatHash(hash, 24)}</span>
                                            {isVerifying && index < currentStep && (
                                                <CheckCircle2 className="w-4 h-4 text-success-500 ml-2" />
                                            )}
                                        </div>
                                    </motion.div>
                                ))}
                            </div>
                        </div>

                        {/* Verify Button */}
                        <div className="flex items-center justify-between pt-4 border-t border-green-500/30">
                            <div>
                                {verificationResult !== null && (
                                    <motion.div
                                        initial={{ opacity: 0, scale: 0.9 }}
                                        animate={{ opacity: 1, scale: 1 }}
                                        className="flex items-center space-x-2"
                                    >
                                        {verificationResult ? (
                                            <>
                                                <CheckCircle2 className="w-6 h-6 text-success-500" />
                                                <span className="text-success-500 font-semibold">Proof Verified ✓</span>
                                            </>
                                        ) : (
                                            <>
                                                <XCircle className="w-6 h-6 text-danger-500" />
                                                <span className="text-danger-500 font-semibold">Verification Failed ✗</span>
                                            </>
                                        )}
                                    </motion.div>
                                )}
                            </div>
                            <button
                                onClick={handleVerifyProof}
                                disabled={isVerifying}
                                className="btn-success px-6"
                            >
                                {isVerifying ? (
                                    <>
                                        <Loader2 className="w-5 h-5 mr-2 animate-spin" />
                                        Verifying... ({currentStep}/{currentProof.proof.length})
                                    </>
                                ) : (
                                    <>
                                        <Hash className="w-5 h-5 mr-2" />
                                        Verify Proof
                                    </>
                                )}
                            </button>
                        </div>
                    </motion.div>
                )}
            </AnimatePresence>
        </div>
    );
};
