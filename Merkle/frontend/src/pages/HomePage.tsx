import React from 'react';
import { Dashboard } from '../components/Dashboard/Dashboard';
import { TreeVisualizationAnimated } from '../components/TreeVisualization/TreeVisualizationAnimated';
import { ProofGenerator } from '../components/ProofVisualization/ProofGenerator';
import { DatasetSelector } from '../components/DatasetSelector/DatasetSelector';
import { OperationsSidebar } from '../components/Sidebar/OperationsSidebar';
import { NodeDetailsModal } from '../components/NodeDetails/NodeDetailsModal';

export const HomePage: React.FC = () => {
    return (
        <>
            <OperationsSidebar />
            <div className="ml-80 space-y-8 p-6">
                {/* Dataset Selection */}
                <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
                    <div className="lg:col-span-1">
                        <DatasetSelector />
                    </div>
                    <div className="lg:col-span-2">
                        <Dashboard />
                    </div>
                </div>

                {/* Tree Visualization */}
                <div className="h-[600px]">
                    <TreeVisualizationAnimated />
                </div>

                {/* Proof Generator */}
                <ProofGenerator />
            </div>
            
            {/* Node Details Modal */}
            <NodeDetailsModal />
        </>
    );
};
