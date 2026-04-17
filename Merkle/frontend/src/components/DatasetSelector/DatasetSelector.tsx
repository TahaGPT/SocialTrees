import React, { useEffect, useState } from 'react';
import { useMerkleStore } from '../../stores/merkleStore';
import api from '../../services/api';
import { Database, Loader2, CheckCircle2 } from 'lucide-react';
import { API_CONFIG, ANIMATION_CONFIG } from '../../config/constants';

export const DatasetSelector: React.FC = () => {
    const { datasets, currentDataset, error, setDatasets, setCurrentDataset, setIsLoading } = useMerkleStore();
    const [isBuilding, setIsBuilding] = useState(false);
    const [buildProgress, setBuildProgress] = useState('');
    const [isLoadingDatasets, setIsLoadingDatasets] = useState(true);
    const [datasetsError, setDatasetsError] = useState<string | null>(null);

    useEffect(() => {
        // First check if backend is accessible
        checkBackendHealth().then((isHealthy) => {
            if (!isHealthy) {
                setDatasetsError(`Backend server is not accessible. Make sure the API server is running on ${API_CONFIG.BASE_URL.replace('/api', '')}`);
                setIsLoadingDatasets(false);
            } else {
                loadDatasets();
            }
        }).catch(() => {
            // Health check failed, but still try to load datasets (might be a different issue)
            loadDatasets();
        });
    }, []);

    const checkBackendHealth = async (): Promise<boolean> => {
        try {
            console.log('[DatasetSelector] Checking backend health...');
            const health = await api.health();
            console.log('[DatasetSelector] Backend health check:', health);
            return true;
        } catch (error: any) {
            console.error('[DatasetSelector] Backend health check failed:', error);
            console.error('[DatasetSelector] Health check error details:', {
                message: error.message,
                code: error.code,
                response: error.response?.data,
                status: error.response?.status,
                config: {
                    url: error.config?.url,
                    method: error.config?.method,
                    baseURL: error.config?.baseURL
                }
            });
            
            // Provide more specific error message
            if (error.code === 'ECONNREFUSED' || error.message?.includes('Network Error')) {
                setDatasetsError(`Cannot connect to backend server. Make sure the API server is running on ${API_CONFIG.BASE_URL.replace('/api', '')}`);
            } else if (error.response?.status === 0) {
                setDatasetsError('CORS error: Backend server is running but blocking requests. Check CORS configuration.');
            } else {
                setDatasetsError(`Backend connection failed: ${error.message || 'Unknown error'}`);
            }
            return false;
        }
    };

    const loadDatasets = async () => {
        console.log('[DatasetSelector] Loading available datasets...');
        setIsLoadingDatasets(true);
        
        // Only clear error if we're starting a fresh load (not from health check failure)
        if (!datasetsError || !datasetsError.includes('not accessible')) {
            setDatasetsError(null);
        }
        
        try {
            const datasetList = await api.listDatasets();
            console.log('[DatasetSelector] Datasets loaded:', datasetList);
            console.log('[DatasetSelector] Number of datasets:', datasetList.length);
            
            if (datasetList.length === 0) {
                console.warn('[DatasetSelector] No datasets found in response');
                setDatasetsError('No datasets found. Make sure the backend server is running and the Dataset folder contains JSON files.');
            } else {
                setDatasetsError(null);
            }
            
            setDatasets(datasetList);
        } catch (error: any) {
            console.error('[DatasetSelector] Failed to load datasets:', error);
            console.error('[DatasetSelector] Error details:', {
                message: error.message,
                response: error.response?.data,
                status: error.response?.status,
                code: error.code
            });
            
            let errorMessage = 'Failed to load datasets. ';
            
            if (error.code === 'ECONNREFUSED' || error.message?.includes('Network Error')) {
                errorMessage += 'Cannot connect to backend server. Make sure the API server is running on http://localhost:8080';
            } else if (error.response?.status === 404) {
                errorMessage += 'API endpoint not found. Check if the backend server is running.';
            } else if (error.response?.status) {
                errorMessage += `Server returned error ${error.response.status}: ${error.response.data?.error || error.message}`;
            } else {
                errorMessage += error.message || 'Unknown error occurred';
            }
            
            setDatasetsError(errorMessage);
            useMerkleStore.setState({ error: errorMessage });
        } finally {
            setIsLoadingDatasets(false);
        }
    };

    const handleBuildTree = async (datasetName: string) => {
        console.log('[DatasetSelector] Building tree for dataset:', datasetName);
        setIsBuilding(true);
        setBuildProgress(`Loading dataset: ${datasetName}...`);
        setIsLoading(true);

        try {
            console.log('[DatasetSelector] Calling API: buildTree(', datasetName, ')');
            const response = await api.buildTree(datasetName);
            console.log('[DatasetSelector] Build response:', response);

            if (response.success) {
                setBuildProgress('Building tree structure...');
                setCurrentDataset(datasetName);
                console.log('[DatasetSelector] Dataset set to:', datasetName);

                // Load tree structure
                console.log('[DatasetSelector] Calling API: getTreeStructure(7)');
                const structure = await api.getTreeStructure(7);
                console.log('[DatasetSelector] Tree structure received:', {
                    nodes: structure.nodes?.length || 0,
                    depth: structure.depth
                });

                console.log('[DatasetSelector] Calling API: getStats()');
                const stats = await api.getStats();
                console.log('[DatasetSelector] Stats received:', stats);

                useMerkleStore.setState({
                    treeStructure: structure,
                    stats: stats,
                });

                setBuildProgress('Complete!');
                console.log('[DatasetSelector] Tree build complete for:', datasetName);
            } else {
                const errorMsg = response.error || 'Failed to build tree';
                console.error('[DatasetSelector] Build failed:', errorMsg);
                useMerkleStore.setState({ error: errorMsg });
            }
        } catch (err: any) {
            console.error('[DatasetSelector] Build failed with exception:', err);
            console.error('[DatasetSelector] Error details:', {
                message: err.message,
                response: err.response?.data,
                status: err.response?.status,
                config: err.config
            });
            let errorMessage = 'Failed to build tree';

            if (err.response && err.response.data && err.response.data.error) {
                errorMessage = err.response.data.error;
            } else if (err.message) {
                errorMessage = err.message;
            }

            useMerkleStore.setState({ error: errorMessage });
        } finally {
            setIsBuilding(false);
            setIsLoading(false);
            setBuildProgress('');
            setTimeout(() => setBuildProgress(''), ANIMATION_CONFIG.BUILD_PROGRESS_CLEAR_DELAY);
        }
    };

    return (
        <div className="glass-card p-6">
            <div className="flex items-center space-x-3 mb-6">
                <Database className="w-6 h-6 text-primary-500" />
                <h3 className="text-xl font-semibold text-green-400">Select Dataset</h3>
            </div>

            {isLoadingDatasets ? (
                <div className="text-center py-8">
                    <Loader2 className="w-8 h-8 mx-auto mb-3 text-primary-500 animate-spin" />
                    <p className="text-green-600">Loading datasets...</p>
                    <p className="text-green-4000 text-sm mt-2">Connecting to backend server...</p>
                </div>
            ) : datasetsError ? (
                <div className="text-center py-8">
                    <div className="mb-4 p-4 bg-danger-500/10 border border-danger-500/30 rounded-lg">
                        <p className="text-danger-400 font-medium mb-2">Failed to Load Datasets</p>
                        <p className="text-danger-300 text-sm">{datasetsError}</p>
                    </div>
                    <button
                        onClick={loadDatasets}
                        className="btn-primary px-4 py-2"
                    >
                        <Loader2 className="w-4 h-4 mr-2 inline" />
                        Retry
                    </button>
                    <div className="mt-4 p-3 bg-black/50 rounded text-left text-sm text-green-600">
                        <p className="font-medium mb-2">Troubleshooting:</p>
                        <ul className="list-disc list-inside space-y-1">
                            <li>Make sure the backend server is running</li>
                            <li>Check that it's running on {API_CONFIG.BASE_URL.replace('/api', '')}</li>
                            <li>Verify the Dataset folder contains JSON files</li>
                            <li>Check the browser console for detailed error messages</li>
                        </ul>
                    </div>
                </div>
            ) : datasets.length === 0 ? (
                <div className="text-center py-8">
                    <Database className="w-12 h-12 mx-auto mb-3 text-green-800" />
                    <p className="text-green-600 font-medium">No Datasets Found</p>
                    <p className="text-green-4000 text-sm mt-2">The Dataset folder appears to be empty.</p>
                    <button
                        onClick={loadDatasets}
                        className="btn-secondary mt-4 px-4 py-2"
                    >
                        Refresh
                    </button>
                </div>
            ) : (
                <div className="space-y-3">
                    {datasets.map((dataset) => (
                        <button
                            key={dataset.name}
                            onClick={() => handleBuildTree(dataset.name)}
                            disabled={isBuilding}
                            className={`w-full p-4 rounded-lg border-2 transition-all text-left ${currentDataset === dataset.name
                                ? 'border-primary-500 bg-primary-500/10'
                                : 'border-green-500/30 bg-black/50 hover:border-green-500/50'
                                }`}
                        >
                            <div className="flex items-center justify-between">
                                <div className="flex-1">
                                    <div className="font-medium text-green-300">{dataset.name}</div>
                                    <div className="text-sm text-green-600 mt-1">{dataset.path}</div>
                                </div>
                                {currentDataset === dataset.name && (
                                    <CheckCircle2 className="w-5 h-5 text-primary-500 ml-3" />
                                )}
                            </div>
                        </button>
                    ))}
                </div>
            )}

            {error && (
                <div className="mb-4 p-3 bg-danger-500/10 border border-danger-500/30 rounded-lg flex items-center text-danger-400 text-sm">
                    <span className="mr-2">⚠️</span>
                    {error}
                </div>
            )}

            {isBuilding && (
                <div className="mt-6 p-4 bg-primary-500/10 border border-primary-500/30 rounded-lg">
                    <div className="flex items-center space-x-3">
                        <Loader2 className="w-5 h-5 text-primary-500 animate-spin" />
                        <span className="text-primary-400 font-medium">{buildProgress}</span>
                    </div>
                </div>
            )}
        </div>
    );
};
