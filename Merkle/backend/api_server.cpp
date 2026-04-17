#include "httplib.h"
#include "merkle_api.hpp"
#include "config.h"
#include <iostream>

using namespace httplib;
using namespace std;

// Global API instance
MerkleAPI api;

void setCorsHeaders(const Request& req, Response& res) {
    string origin = req.get_header_value("Origin");
    if (origin.empty()) {
        origin = "*";
    }
    res.set_header("Access-Control-Allow-Origin", origin);
    res.set_header("Access-Control-Allow-Credentials", "true");
}

int main() {
    // Ensure logs are flushed immediately
    cout.setf(ios::unitbuf);
    
    Server svr;
    
    // Logger to see all requests
    svr.set_logger([](const Request& req, const Response& res) {
        cout << "[SERVER] " << req.method << " " << req.path << " -> " << res.status << endl;
    });

    // Enable CORS for frontend (Dynamic Origin)
    // Note: We can't set dynamic headers in set_default_headers easily with cpp-httplib
    // So we will set them in the handlers or use a middleware approach if possible.
    // For now, we'll use * for default, but the OPTIONS handler is the critical one for preflight.
    // Actually, let's remove default headers and set them in a pre-routing handler if possible, 
    // or just rely on the fact that for GET/POST, if preflight succeeds, * usually works.
    // BUT, if we use Credentials=true, we MUST NOT use *.
    
    // Let's use a "Post-Routing" hook or just set headers in every response?
    // cpp-httplib doesn't have global middleware easily.
    // We'll set a logger that ALSO sets headers? No, logger is const Response.
    
    // We will set default headers to * for now, but for the critical POST, we might need to be specific.
    // Let's try * first for default, but with Credentials=true, it might fail.
    // Safest is to NOT set default headers here and let OPTIONS handle it, 
    // and for actual responses, we add a helper or just set it.
    
    // Actually, we can use set_pre_routing_handler? No.
    
    // Let's just update the default to * and hope the browser accepts it for the actual request 
    // if preflight was specific. 
    // Wait, if preflight returns specific origin, the actual request must also match.
    
    // Let's remove set_default_headers and handle it manually or use a lambda wrapper.
    // Or just set it to * and see. 
    // Actually, if we set Credentials=true, * is invalid.
    
    // Let's use a simple approach: Set headers in the logger? No.
    // We'll just add the headers in the endpoints.
    // Note: We don't set default CORS headers here because we set them explicitly
    // in each handler to avoid duplicate headers which browsers reject.
    
    // Handle OPTIONS requests (CORS preflight) - Explicit paths to ensure matching
    auto optionsHandler = [](const Request& req, Response& res) {
        // Log headers to debug CORS
        cout << "[SERVER] OPTIONS Headers for " << req.path << ":" << endl;
        for (const auto& h : req.headers) {
            cout << "  " << h.first << ": " << h.second << endl;
        }

        string origin = req.get_header_value("Origin");
        if (origin.empty()) {
            origin = "*";
        }
        
        res.set_header("Access-Control-Allow-Origin", origin);
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS, PUT, DELETE");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, Accept, Origin, X-Requested-With");
        res.set_header("Access-Control-Allow-Credentials", "true");
        res.set_header("Access-Control-Max-Age", "86400");
        res.status = 200;
        res.set_content("", "text/plain");
    };

    svr.Options("/api/datasets", optionsHandler);
    svr.Options("/api/tree/build", optionsHandler);
    svr.Options("/api/tree/structure", optionsHandler);
    svr.Options("/api/tree/root", optionsHandler);
    svr.Options("/api/proof/generate", optionsHandler);
    svr.Options("/api/proof/verify", optionsHandler);
    svr.Options("/api/stats", optionsHandler);
    svr.Options("/api/history", optionsHandler);
    svr.Options("/api/health", optionsHandler);
    svr.Options("/api/record/update", optionsHandler);
    svr.Options("/api/record/delete", optionsHandler);
    svr.Options("/api/record/insert", optionsHandler);
    svr.Options("/api/batch/update", optionsHandler);
    // Catch-all for other paths
    svr.Options(".*", optionsHandler);
    
    // ==========================================
    // API ENDPOINTS
    // ==========================================
    
    // GET /api/datasets - List available datasets
    svr.Get("/api/datasets", [](const Request& req, Response& res) {
        setCorsHeaders(req, res);
        json response = api.listDatasets();
        res.set_content(response.dump(2), "application/json");
    });
    
    // POST /api/tree/build - Build tree for dataset
    svr.Post("/api/tree/build", [](const Request& req, Response& res) {
        setCorsHeaders(req, res);
        cout << "[SERVER] Received POST /api/tree/build" << endl;
        try {
            json body = json::parse(req.body);
            string datasetName = body["dataset"];
            cout << "[SERVER] Requesting build for: " << datasetName << endl;
            
            json response = api.buildTree(datasetName);
            res.set_content(response.dump(2), "application/json");
        } catch (const exception& e) {
            cout << "[SERVER] Exception in handler: " << e.what() << endl;
            json error = {
                {"success", false},
                {"error", e.what()}
            };
            res.set_content(error.dump(2), "application/json");
        }
    });
    
    // GET /api/tree/structure - Get tree structure
    svr.Get("/api/tree/structure", [](const Request& req, Response& res) {
        setCorsHeaders(req, res);
        int maxDepth = 7;
        if (req.has_param("maxDepth")) {
            maxDepth = stoi(req.get_param_value("maxDepth"));
        }
        
        json response = api.getTreeStructure(maxDepth);
        res.set_content(response.dump(2), "application/json");
    });
    
    // GET /api/tree/root - Get root hash
    svr.Get("/api/tree/root", [](const Request& req, Response& res) {
        setCorsHeaders(req, res);
        json response = api.getRootHash();
        res.set_content(response.dump(2), "application/json");
    });
    
    // POST /api/proof/generate - Generate proof
    svr.Post("/api/proof/generate", [](const Request& req, Response& res) {
        setCorsHeaders(req, res);
        try {
            json body = json::parse(req.body);
            string recordId = body["recordId"];
            
            json response = api.generateProof(recordId);
            res.set_content(response.dump(2), "application/json");
        } catch (const exception& e) {
            json error = {
                {"success", false},
                {"error", e.what()}
            };
            res.set_content(error.dump(2), "application/json");
        }
    });
    
    // POST /api/proof/generate-verify - Generate and verify proof (optimized)
    svr.Post("/api/proof/generate-verify", [](const Request& req, Response& res) {
        setCorsHeaders(req, res);
        try {
            json body = json::parse(req.body);
            string recordId = body["recordId"];
            
            json response = api.generateAndVerifyProof(recordId);
            res.set_content(response.dump(2), "application/json");
        } catch (const exception& e) {
            json error = {
                {"success", false},
                {"error", e.what()}
            };
            res.set_content(error.dump(2), "application/json");
        }
    });
    
    // OPTIONS handler for generate-verify
    svr.Options("/api/proof/generate-verify", optionsHandler);
    
    // POST /api/proof/verify - Verify proof
    svr.Post("/api/proof/verify", [](const Request& req, Response& res) {
        setCorsHeaders(req, res);
        try {
            json body = json::parse(req.body);
            json response = api.verifyProof(body);
            res.set_content(response.dump(2), "application/json");
        } catch (const exception& e) {
            json error = {
                {"success", false},
                {"error", e.what()}
            };
            res.set_content(error.dump(2), "application/json");
        }
    });
    
    // GET /api/stats - Get statistics
    svr.Get("/api/stats", [](const Request& req, Response& res) {
        setCorsHeaders(req, res);
        json response = api.getStats();
        res.set_content(response.dump(2), "application/json");
    });
    
    // GET /api/history - Get root history
    svr.Get("/api/history", [](const Request& req, Response& res) {
        setCorsHeaders(req, res);
        json response = api.getRootHistory();
        res.set_content(response.dump(2), "application/json");
    });
    
    // POST /api/record/update - Update a record
    svr.Post("/api/record/update", [](const Request& req, Response& res) {
        setCorsHeaders(req, res);
        try {
            json body = json::parse(req.body);
            string recordId = body["recordId"];
            json response = api.updateRecord(recordId, body["newData"]);
            res.set_content(response.dump(2), "application/json");
        } catch (const exception& e) {
            json error = {
                {"success", false},
                {"error", e.what()}
            };
            res.set_content(error.dump(2), "application/json");
        }
    });
    
    // POST /api/record/delete - Delete a record
    svr.Post("/api/record/delete", [](const Request& req, Response& res) {
        setCorsHeaders(req, res);
        try {
            json body = json::parse(req.body);
            string recordId = body["recordId"];
            json response = api.deleteRecord(recordId);
            res.set_content(response.dump(2), "application/json");
        } catch (const exception& e) {
            json error = {
                {"success", false},
                {"error", e.what()}
            };
            res.set_content(error.dump(2), "application/json");
        }
    });
    
    // POST /api/record/insert - Insert a new record
    svr.Post("/api/record/insert", [](const Request& req, Response& res) {
        setCorsHeaders(req, res);
        try {
            json body = json::parse(req.body);
            json response = api.insertRecord(body["recordData"]);
            res.set_content(response.dump(2), "application/json");
        } catch (const exception& e) {
            json error = {
                {"success", false},
                {"error", e.what()}
            };
            res.set_content(error.dump(2), "application/json");
        }
    });
    
    // POST /api/batch/update - Batch update records
    svr.Post("/api/batch/update", [](const Request& req, Response& res) {
        setCorsHeaders(req, res);
        try {
            json body = json::parse(req.body);
            json response = api.batchUpdate(body["updates"]);
            res.set_content(response.dump(2), "application/json");
        } catch (const exception& e) {
            json error = {
                {"success", false},
                {"error", e.what()}
            };
            res.set_content(error.dump(2), "application/json");
        }
    });
    
    // Health check
    svr.Get("/api/health", [](const Request& req, Response& res) {
        setCorsHeaders(req, res);
        json response = {
            {"status", "ok"},
            {"service", "Merkle Tree API"},
            {"version", "1.0.0"}
        };
        res.set_content(response.dump(2), "application/json");
    });
    
    // ==========================================
    // START SERVER
    // ==========================================
    
    cout << "========================================" << endl;
    cout << "  Merkle Tree API Server" << endl;
    cout << "========================================" << endl;
    cout << "Server running on http://" << Config::SERVER_HOST << ":" << Config::SERVER_PORT << endl;
    cout << "API Endpoints:" << endl;
    cout << "  GET  /api/datasets" << endl;
    cout << "  POST /api/tree/build" << endl;
    cout << "  GET  /api/tree/structure" << endl;
    cout << "  GET  /api/tree/root" << endl;
    cout << "  POST /api/proof/generate" << endl;
    cout << "  POST /api/proof/generate-verify (optimized)" << endl;
    cout << "  POST /api/proof/verify" << endl;
    cout << "  GET  /api/stats" << endl;
    cout << "  GET  /api/history" << endl;
    cout << "  POST /api/record/update" << endl;
    cout << "  POST /api/record/delete" << endl;
    cout << "  POST /api/record/insert" << endl;
    cout << "  POST /api/batch/update" << endl;
    cout << "  GET  /api/health" << endl;
    cout << "========================================" << endl;
    
    svr.listen(Config::SERVER_HOST.c_str(), Config::SERVER_PORT);
    
    return 0;
}
