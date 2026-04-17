import React, { useEffect, useRef, useState } from 'react';
import * as d3 from 'd3';
import { useMerkleStore } from '../../stores/merkleStore';
import { ZoomIn, ZoomOut, Maximize2, Download } from 'lucide-react';
import { getNodeColor } from '../../lib/utils';
import { ANIMATION_CONFIG, TREE_CONFIG } from '../../config/constants';
import api from '../../services/api';

interface D3Node extends d3.HierarchyPointNode<any> {
    x: number;
    y: number;
}

interface AnimationState {
    modifiedNodes: Set<number>;
    addedNodes: Set<number>;
    deletedNodes: Set<number>;
}

export const TreeVisualizationAnimated: React.FC = () => {
    const svgRef = useRef<SVGSVGElement>(null);
    const containerRef = useRef<HTMLDivElement>(null);
    const zoomBehaviorRef = useRef<any>(null);
    const { treeStructure, selectedNode, proofPath, currentDataset } = useMerkleStore();
    const [zoom, setZoom] = useState(1);
    const [animationState, setAnimationState] = useState<AnimationState>({
        modifiedNodes: new Set(),
        addedNodes: new Set(),
        deletedNodes: new Set(),
    });
    const previousTreeRef = useRef<any>(null);

    useEffect(() => {
        if (!treeStructure || !svgRef.current) {
            return;
        }

        // Detect changes
        if (previousTreeRef.current) {
            detectChanges(previousTreeRef.current, treeStructure);
        }

        renderTree();
        previousTreeRef.current = treeStructure;
    }, [treeStructure, selectedNode, proofPath]);

    const detectChanges = (oldTree: any, newTree: any) => {
        const modified = new Set<number>();
        const added = new Set<number>();
        const deleted = new Set<number>();

        // Compare nodes
        const oldNodes = new Map(oldTree.nodes.map((n: any) => [n.index, n]));
        const newNodes = new Map(newTree.nodes.map((n: any) => [n.index, n]));

        // Find modified and added nodes
        newNodes.forEach((node: any, index: any) => {
            const nodeIndex = Number(index);
            const oldNode: any = oldNodes.get(nodeIndex);
            if (!oldNode) {
                added.add(nodeIndex);
            } else if (oldNode.hash !== node.hash) {
                modified.add(nodeIndex);
            }
        });

        // Find deleted nodes
        oldNodes.forEach((_node: any, index: any) => {
            const nodeIndex = Number(index);
            if (!newNodes.has(nodeIndex)) {
                deleted.add(nodeIndex);
            }
        });

        setAnimationState({ modifiedNodes: modified, addedNodes: added, deletedNodes: deleted });

        // Clear animation state after configured duration
        setTimeout(() => {
            setAnimationState({ modifiedNodes: new Set(), addedNodes: new Set(), deletedNodes: new Set() });
        }, ANIMATION_CONFIG.HIGHLIGHT_DURATION);
    };

    const renderTree = () => {
        if (!svgRef.current || !containerRef.current) return;

        const width = containerRef.current.clientWidth;
        const height = containerRef.current.clientHeight;

        // Clear previous render
        d3.select(svgRef.current).selectAll('*').remove();

        const svg = d3.select(svgRef.current)
            .attr('width', width)
            .attr('height', height);

        // Add black background
        svg.append('rect')
            .attr('width', width)
            .attr('height', height)
            .attr('fill', '#000000');

        const g = svg.append('g')
            .attr('transform', `translate(${width / 2}, 50)`);

        // Create hierarchy from tree structure
        const root = d3.hierarchy(createHierarchy());

        // Create tree layout
        const treeLayout = d3.tree<any>()
            .size([width - 100, height - 100])
            .separation((a, b) => (a.parent === b.parent ? 1 : 2) / a.depth);

        treeLayout(root);

        // Draw links with animation
        g.selectAll('.link')
            .data(root.links())
            .enter()
            .append('path')
            .attr('class', 'link')
            .attr('d', d3.linkVertical<any, D3Node>()
                .x(d => d.x)
                .y(d => d.y))
            .attr('fill', 'none')
            .attr('stroke', (d: any) => {
                const isInProofPath = proofPath.includes(d.target.data.index);
                const isModified = animationState.modifiedNodes.has(d.target.data.index);
                if (isModified) return getNodeColor('modified');
                if (isInProofPath) return getNodeColor('proof');
                return '#00FF00'; // Neon green
            })
            .attr('stroke-width', (d: any) => {
                const isInProofPath = proofPath.includes(d.target.data.index);
                return isInProofPath ? 3 : 1.5;
            })
            .attr('opacity', 0)
            .transition()
            .duration(ANIMATION_CONFIG.DURATION)
            .attr('opacity', 0.6);

        // Draw nodes
        const nodes = g.selectAll('.node')
            .data(root.descendants())
            .enter()
            .append('g')
            .attr('class', 'node')
            .attr('transform', (d: any) => `translate(${d.x},${d.y})`)
            .style('cursor', 'pointer')
            .on('click', (_event, d: any) => {
                useMerkleStore.setState({ selectedNode: d.data.index });
            });

        // Node circles with animations
        nodes.append('circle')
            .attr('r', 0) // Start from 0 for animation
            .attr('fill', (d: any) => {
                const index = d.data.index;
                const isSelected = selectedNode === index;
                const isInProofPath = proofPath.includes(index);
                const isModified = animationState.modifiedNodes.has(index);
                const isAdded = animationState.addedNodes.has(index);
                const isRoot = d.depth === 0;
                const isLeaf = !d.children;

                if (isSelected) return '#FFFF00'; // Yellow for selected
                if (isModified) return getNodeColor('modified');
                if (isAdded) return getNodeColor('added');
                if (isInProofPath) return getNodeColor('proof');
                if (isRoot) return getNodeColor('root');
                if (isLeaf) return getNodeColor('leaf');
                return getNodeColor('internal');
            })
            .attr('stroke', (d: any) => {
                const isSelected = selectedNode === d.data.index;
                return isSelected ? '#FFFFFF' : '#00FF00';
            })
            .attr('stroke-width', (d: any) => {
                const isSelected = selectedNode === d.data.index;
                const isModified = animationState.modifiedNodes.has(d.data.index);
                return isSelected || isModified ? 3 : 2;
            })
            .attr('opacity', 0.9)
            .transition()
            .duration(ANIMATION_CONFIG.DURATION)
            .attr('r', (d: any) => {
                if (d.depth === 0) return 12;
                if (d.children) return 8;
                return 6;
            });

        // Add pulsing animation for modified nodes
        nodes.filter((d: any) => animationState.modifiedNodes.has(d.data.index))
            .select('circle')
            .transition()
            .duration(ANIMATION_CONFIG.DURATION)
            .attr('r', 15)
            .transition()
            .duration(ANIMATION_CONFIG.DURATION)
            .attr('r', (d: any) => {
                if (d.depth === 0) return 12;
                if (d.children) return 8;
                return 6;
            });

        // Hover effects
        nodes.on('mouseover', function () {
            d3.select(this).select('circle')
                .transition()
                .duration(ANIMATION_CONFIG.TRANSITION_DURATION)
                .attr('r', (d: any) => {
                    if (d.depth === 0) return 14;
                    if (d.children) return 10;
                    return 8;
                })
                .attr('filter', 'url(#glow)');
        })
        .on('mouseout', function () {
            d3.select(this).select('circle')
                .transition()
                .duration(ANIMATION_CONFIG.TRANSITION_DURATION)
                .attr('r', (d: any) => {
                    if (d.depth === 0) return 12;
                    if (d.children) return 8;
                    return 6;
                })
                .attr('filter', null);
        })
        .on('click', async function(event, d: any) {
            event.stopPropagation();
            const nodeIndex = d.data.index;
            const nodeData = d.data;
            
            // Set selected node for highlighting
            useMerkleStore.setState({ selectedNode: nodeIndex });
            
            // Check if it's a leaf node (no children in the hierarchy)
            const isLeafNode = !d.children || d.children.length === 0;
            
            // For leaf nodes, try to fetch full record details if we have a reviewId
            // If no reviewId, it means this node is structurally a leaf but doesn't have data
            if (isLeafNode) {
                if (nodeData.reviewId) {
                    try {
                        const response = await api.getNodeDetails(nodeData.reviewId);
                        if (response.success && response.record) {
                            useMerkleStore.setState({
                                selectedNodeDetails: {
                                    nodeIndex,
                                    isLeaf: isLeafNode,
                                    nodeData: {
                                        ...nodeData,
                                        record: response.record
                                    }
                                }
                            });
                        } else {
                            // Show basic node info even if fetch fails
                            useMerkleStore.setState({
                                selectedNodeDetails: {
                                    nodeIndex,
                                    isLeaf: isLeafNode,
                                    nodeData
                                }
                            });
                        }
                    } catch (error) {
                        console.error('Error fetching node details:', error);
                        // Show basic node info on error
                        useMerkleStore.setState({
                            selectedNodeDetails: {
                                nodeIndex,
                                isLeaf: isLeafNode,
                                nodeData
                            }
                        });
                    }
                } else {
                    // Leaf node without reviewId - show with message
                    useMerkleStore.setState({
                        selectedNodeDetails: {
                            nodeIndex,
                            isLeaf: isLeafNode,
                            nodeData,
                            noData: true
                        }
                    });
                }
            } else {
                // For internal/root nodes, show basic info
                useMerkleStore.setState({
                    selectedNodeDetails: {
                        nodeIndex,
                        isLeaf: isLeafNode,
                        nodeData
                    }
                });
            }
        })
        .style('cursor', 'pointer');

        // Node labels (index or reviewID)
        nodes.append('text')
            .attr('dy', -15)
            .attr('text-anchor', 'middle')
            .attr('fill', '#00FF00') // Neon green text
            .attr('font-size', (d: any) => d.depth === 0 ? '11px' : '9px')
            .attr('font-family', 'monospace')
            .attr('font-weight', (d: any) => d.depth === 0 ? 'bold' : 'normal')
            .text((d: any) => {
                // For root node
                if (d.depth === 0) return `Root`;
                
                // For leaf nodes, show reviewID if available
                const isLeaf = !d.children || d.children.length === 0;
                if (isLeaf && d.data.reviewId) {
                    // Show compact version of reviewID
                    const parts = d.data.reviewId.split('_');
                    if (parts.length >= 2) {
                        return `${parts[0].slice(-4)}_${parts[1]}`;
                    }
                    return d.data.reviewId.slice(-8);
                }
                
                // For internal nodes or nodes without reviewID, show index
                return `#${d.data.index}`;
            })
            .attr('opacity', 0)
            .transition()
            .duration(ANIMATION_CONFIG.DURATION)
            .attr('opacity', 0.9);

        // Add glow filter for neon effect
        const defs = svg.append('defs');
        const filter = defs.append('filter')
            .attr('id', 'glow');
        filter.append('feGaussianBlur')
            .attr('stdDeviation', '3')
            .attr('result', 'coloredBlur');
        const feMerge = filter.append('feMerge');
        feMerge.append('feMergeNode')
            .attr('in', 'coloredBlur');
        feMerge.append('feMergeNode')
            .attr('in', 'SourceGraphic');

        // Add zoom behavior
        const zoomBehavior = d3.zoom<SVGSVGElement, unknown>()
            .scaleExtent([0.5, 3])
            .on('zoom', (event) => {
                g.attr('transform', event.transform);
                setZoom(event.transform.k);
            });

        zoomBehaviorRef.current = zoomBehavior;
        svg.call(zoomBehavior as any);
    };

    const createHierarchy = () => {
        if (!treeStructure) return { index: 0, children: [] };

        // Build tree structure from flat array
        const nodeMap = new Map();
        treeStructure.nodes.forEach(node => {
            nodeMap.set(node.index, { ...node, children: [] });
        });

        // Connect children to parents (quad-tree structure)
        treeStructure.nodes.forEach(node => {
            if (node.index > 0) {
                const parentIndex = Math.floor((node.index - 1) / TREE_CONFIG.ARITY);
                const parent = nodeMap.get(parentIndex);
                if (parent) {
                    parent.children.push(nodeMap.get(node.index));
                }
            }
        });

        return nodeMap.get(0) || { index: 0, children: [] };
    };

    const handleZoomIn = () => {
        if (!svgRef.current || !zoomBehaviorRef.current) return;
        const svg = d3.select(svgRef.current);
        svg.transition().duration(300).call(zoomBehaviorRef.current.scaleBy, 1.3);
    };

    const handleZoomOut = () => {
        if (!svgRef.current || !zoomBehaviorRef.current) return;
        const svg = d3.select(svgRef.current);
        svg.transition().duration(300).call(zoomBehaviorRef.current.scaleBy, 0.7);
    };

    const handleReset = () => {
        if (!svgRef.current || !zoomBehaviorRef.current) return;
        const svg = d3.select(svgRef.current);
        svg.transition().duration(300).call(zoomBehaviorRef.current.transform, d3.zoomIdentity);
    };

    const handleDownload = () => {
        if (!svgRef.current) return;

        const svgData = new XMLSerializer().serializeToString(svgRef.current);
        const blob = new Blob([svgData], { type: 'image/svg+xml' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = 'merkle-tree.svg';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    };

    if (!treeStructure) {
        return (
            <div className="glass-card p-12 text-center h-full flex items-center justify-center bg-black">
                <div>
                    <div className="w-16 h-16 mx-auto mb-4 rounded-full bg-black border-2 border-green-500 flex items-center justify-center">
                        <Maximize2 className="w-8 h-8 text-green-500" />
                    </div>
                    <h3 className="text-xl font-semibold text-green-500 mb-2">
                        No Tree to Display
                    </h3>
                    <p className="text-green-400">
                        Build a tree from a dataset to see the visualization
                    </p>
                    {currentDataset && (
                        <p className="text-yellow-400 mt-2 text-sm">
                            Dataset "{currentDataset}" selected. Click "Build Tree" to visualize.
                        </p>
                    )}
                </div>
            </div>
        );
    }

    return (
        <div className="glass-card p-4 h-full flex flex-col bg-black border border-green-500">
            {/* Controls */}
            <div className="flex items-center justify-between mb-4">
                <div>
                    <h3 className="text-lg font-semibold text-green-500">Tree Visualization</h3>
                    <p className="text-sm text-green-400">
                        Showing {treeStructure.nodes.length} nodes • Depth: {treeStructure.depth}
                    </p>
                </div>
                <div className="flex items-center space-x-2">
                    <span className="text-sm text-green-400">Zoom: {(zoom * 100).toFixed(0)}%</span>
                    <button onClick={handleZoomIn} className="btn-secondary p-2 bg-black border border-green-500 text-green-500 hover:bg-green-900">
                        <ZoomIn className="w-5 h-5" />
                    </button>
                    <button onClick={handleZoomOut} className="btn-secondary p-2 bg-black border border-green-500 text-green-500 hover:bg-green-900">
                        <ZoomOut className="w-5 h-5" />
                    </button>
                    <button onClick={handleReset} className="btn-secondary p-2 bg-black border border-green-500 text-green-500 hover:bg-green-900">
                        <Maximize2 className="w-5 h-5" />
                    </button>
                    <button onClick={handleDownload} className="btn-secondary p-2 bg-black border border-green-500 text-green-500 hover:bg-green-900">
                        <Download className="w-5 h-5" />
                    </button>
                </div>
            </div>

            {/* Legend */}
            <div className="flex items-center space-x-6 mb-4 text-sm">
                <div className="flex items-center space-x-2">
                    <div className="w-3 h-3 rounded-full" style={{ backgroundColor: getNodeColor('root') }}></div>
                    <span className="text-green-400">Root</span>
                </div>
                <div className="flex items-center space-x-2">
                    <div className="w-3 h-3 rounded-full" style={{ backgroundColor: getNodeColor('internal') }}></div>
                    <span className="text-green-400">Internal</span>
                </div>
                <div className="flex items-center space-x-2">
                    <div className="w-3 h-3 rounded-full" style={{ backgroundColor: getNodeColor('leaf') }}></div>
                    <span className="text-green-400">Leaf</span>
                </div>
                <div className="flex items-center space-x-2">
                    <div className="w-3 h-3 rounded-full" style={{ backgroundColor: getNodeColor('proof') }}></div>
                    <span className="text-green-400">Proof Path</span>
                </div>
                <div className="flex items-center space-x-2">
                    <div className="w-3 h-3 rounded-full" style={{ backgroundColor: getNodeColor('modified') }}></div>
                    <span className="text-green-400">Modified</span>
                </div>
            </div>

            {/* SVG Container */}
            <div ref={containerRef} className="flex-1 bg-black rounded-lg overflow-hidden border border-green-500">
                <svg ref={svgRef} className="w-full h-full"></svg>
            </div>
        </div>
    );
};
