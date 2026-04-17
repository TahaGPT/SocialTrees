import React, { useEffect, useRef, useState } from 'react';
import * as d3 from 'd3';
import { useMerkleStore } from '../../stores/merkleStore';
import { ZoomIn, ZoomOut, Maximize2, Download } from 'lucide-react';
import { getNodeColor } from '../../lib/utils';

interface D3Node extends d3.HierarchyPointNode<any> {
    x: number;
    y: number;
}

export const TreeVisualization: React.FC = () => {
    const svgRef = useRef<SVGSVGElement>(null);
    const containerRef = useRef<HTMLDivElement>(null);
    const { treeStructure, selectedNode, proofPath, currentDataset } = useMerkleStore();
    const [zoom, setZoom] = useState(1);

    useEffect(() => {
        console.log('[TreeVisualization] Effect triggered - treeStructure:', treeStructure ? 'exists' : 'null', 'nodes:', treeStructure?.nodes?.length || 0);
        if (!treeStructure || !svgRef.current) {
            console.log('[TreeVisualization] Skipping render - treeStructure:', !treeStructure, 'svgRef:', !svgRef.current);
            return;
        }

        console.log('[TreeVisualization] Rendering tree with', treeStructure.nodes.length, 'nodes');
        renderTree();
    }, [treeStructure, selectedNode, proofPath]);

    const renderTree = () => {
        if (!svgRef.current || !containerRef.current) return;

        const width = containerRef.current.clientWidth;
        const height = containerRef.current.clientHeight;

        // Clear previous render
        d3.select(svgRef.current).selectAll('*').remove();

        const svg = d3.select(svgRef.current)
            .attr('width', width)
            .attr('height', height);

        const g = svg.append('g')
            .attr('transform', `translate(${width / 2}, 50)`);

        // Create hierarchy from tree structure
        const root = d3.hierarchy(createHierarchy());

        // Create tree layout
        const treeLayout = d3.tree<any>()
            .size([width - 100, height - 100])
            .separation((a, b) => (a.parent === b.parent ? 1 : 2) / a.depth);

        treeLayout(root);

        // Draw links
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
                return isInProofPath ? getNodeColor('proof') : '#475569';
            })
            .attr('stroke-width', (d: any) => {
                const isInProofPath = proofPath.includes(d.target.data.index);
                return isInProofPath ? 3 : 2;
            })
            .attr('opacity', 0.6);

        // Draw nodes
        const nodes = g.selectAll('.node')
            .data(root.descendants())
            .enter()
            .append('g')
            .attr('class', 'node')
            .attr('transform', (d: any) => `translate(${d.x},${d.y})`)
            .style('cursor', 'pointer')
            .on('click', (event, d: any) => {
                useMerkleStore.setState({ selectedNode: d.data.index });
            });

        // Node circles
        nodes.append('circle')
            .attr('r', (d: any) => {
                if (d.depth === 0) return 12;
                if (d.children) return 8;
                return 6;
            })
            .attr('fill', (d: any) => {
                const isSelected = selectedNode === d.data.index;
                const isInProofPath = proofPath.includes(d.data.index);

                if (isSelected) return '#f59e0b';
                if (isInProofPath) return getNodeColor('proof');
                if (d.depth === 0) return getNodeColor('root');
                if (d.children) return getNodeColor('internal');
                return getNodeColor('leaf');
            })
            .attr('stroke', (d: any) => {
                const isSelected = selectedNode === d.data.index;
                return isSelected ? '#fff' : '#1e293b';
            })
            .attr('stroke-width', (d: any) => {
                const isSelected = selectedNode === d.data.index;
                return isSelected ? 3 : 2;
            })
            .attr('opacity', 0.9)
            .on('mouseover', function (event, d: any) {
                d3.select(this)
                    .transition()
                    .duration(200)
                    .attr('r', (d: any) => {
                        if (d.depth === 0) return 14;
                        if (d.children) return 10;
                        return 8;
                    });
            })
            .on('mouseout', function (event, d: any) {
                d3.select(this)
                    .transition()
                    .duration(200)
                    .attr('r', (d: any) => {
                        if (d.depth === 0) return 12;
                        if (d.children) return 8;
                        return 6;
                    });
            });

        // Node labels (index)
        nodes.append('text')
            .attr('dy', -15)
            .attr('text-anchor', 'middle')
            .attr('fill', '#94a3b8')
            .attr('font-size', '10px')
            .attr('font-family', 'monospace')
            .text((d: any) => d.data.index);

        // Add zoom behavior
        const zoomBehavior = d3.zoom<SVGSVGElement, unknown>()
            .scaleExtent([0.5, 3])
            .on('zoom', (event) => {
                g.attr('transform', event.transform);
                setZoom(event.transform.k);
            });

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
                const parentIndex = Math.floor((node.index - 1) / 4);
                const parent = nodeMap.get(parentIndex);
                if (parent) {
                    parent.children.push(nodeMap.get(node.index));
                }
            }
        });

        return nodeMap.get(0) || { index: 0, children: [] };
    };

    const handleZoomIn = () => {
        const svg = d3.select(svgRef.current);
        svg.transition().call(d3.zoom<SVGSVGElement, unknown>().scaleBy as any, 1.3);
    };

    const handleZoomOut = () => {
        const svg = d3.select(svgRef.current);
        svg.transition().call(d3.zoom<SVGSVGElement, unknown>().scaleBy as any, 0.7);
    };

    const handleReset = () => {
        const svg = d3.select(svgRef.current);
        svg.transition().call(d3.zoom<SVGSVGElement, unknown>().transform as any, d3.zoomIdentity);
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
            <div className="glass-card p-12 text-center h-full flex items-center justify-center">
                <div>
                    <div className="w-16 h-16 mx-auto mb-4 rounded-full bg-black flex items-center justify-center">
                        <Maximize2 className="w-8 h-8 text-green-800" />
                    </div>
                    <h3 className="text-xl font-semibold text-green-500 mb-2">
                        No Tree to Display
                    </h3>
                    <p className="text-green-4000">
                        Build a tree from a dataset to see the visualization
                    </p>
                    {currentDataset && (
                        <p className="text-warning-500 mt-2 text-sm">
                            Dataset "{currentDataset}" selected. Click "Build Tree" to visualize.
                        </p>
                    )}
                </div>
            </div>
        );
    }

    return (
        <div className="glass-card p-4 h-full flex flex-col">
            {/* Controls */}
            <div className="flex items-center justify-between mb-4">
                <div>
                    <h3 className="text-lg font-semibold text-green-400">Tree Visualization</h3>
                    <p className="text-sm text-green-600">
                        Showing {treeStructure.nodes.length} nodes • Depth: {treeStructure.depth}
                    </p>
                </div>
                <div className="flex items-center space-x-2">
                    <span className="text-sm text-green-600">Zoom: {(zoom * 100).toFixed(0)}%</span>
                    <button onClick={handleZoomIn} className="btn-secondary p-2">
                        <ZoomIn className="w-5 h-5" />
                    </button>
                    <button onClick={handleZoomOut} className="btn-secondary p-2">
                        <ZoomOut className="w-5 h-5" />
                    </button>
                    <button onClick={handleReset} className="btn-secondary p-2">
                        <Maximize2 className="w-5 h-5" />
                    </button>
                    <button onClick={handleDownload} className="btn-secondary p-2">
                        <Download className="w-5 h-5" />
                    </button>
                </div>
            </div>

            {/* Legend */}
            <div className="flex items-center space-x-6 mb-4 text-sm">
                <div className="flex items-center space-x-2">
                    <div className="w-3 h-3 rounded-full" style={{ backgroundColor: getNodeColor('root') }}></div>
                    <span className="text-green-600">Root</span>
                </div>
                <div className="flex items-center space-x-2">
                    <div className="w-3 h-3 rounded-full" style={{ backgroundColor: getNodeColor('internal') }}></div>
                    <span className="text-green-600">Internal</span>
                </div>
                <div className="flex items-center space-x-2">
                    <div className="w-3 h-3 rounded-full" style={{ backgroundColor: getNodeColor('leaf') }}></div>
                    <span className="text-green-600">Leaf</span>
                </div>
                <div className="flex items-center space-x-2">
                    <div className="w-3 h-3 rounded-full" style={{ backgroundColor: getNodeColor('proof') }}></div>
                    <span className="text-green-600">Proof Path</span>
                </div>
            </div>

            {/* SVG Container */}
            <div ref={containerRef} className="flex-1 bg-black/30 rounded-lg overflow-hidden">
                <svg ref={svgRef} className="w-full h-full"></svg>
            </div>
        </div>
    );
};
