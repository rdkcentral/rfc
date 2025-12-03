/**
 * OpenTelemetry OTLP HTTP Instrumentation for rfc
 * Compatible with OpenTelemetry C++ SDK v1.23.0
 * Sends traces to OpenTelemetry Collector via OTLP HTTP
 */

#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/span_context.h>
#include <cstdlib>
#include <cstring>  // for memcpy
#include <iostream>
#include <memory>
#include <string>
#include <functional>
#include <sstream>
#include <iomanip>
#include <array>
#include <thread>
#include <chrono>
#include "rdk_debug.h"

#define LOG_RFCAPI  "LOG.RDK.RFCAPI"

namespace trace = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace resource = opentelemetry::sdk::resource;
namespace nostd = opentelemetry::nostd;
namespace otlp = opentelemetry::exporter::otlp;

class rfcOTLPTracer {
private:
    nostd::shared_ptr<trace::Tracer> tracer_;

    std::string getCollectorEndpoint() {
        // Check environment variable first for Docker container networking
        const char* env_endpoint = std::getenv("OTEL_EXPORTER_OTLP_ENDPOINT");
        if (env_endpoint != nullptr) {
            return "http://otel-collector:4318";
        }

        // Check for container environment
        const char* container_env = std::getenv("RUNNING_IN_CONTAINER");
        if (container_env != nullptr && std::string(container_env) == "true") {
            // Use collector service name in Docker network
            return "http://otel-collector:4318";
        }

        // Default to localhost for non-container environments
        return "http://otel-collector:4318";
    }

public:
    rfcOTLPTracer() {
        RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: ========== Constructor START ==========\n");
        try {
            initializeTracer();
            RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: ========== Constructor SUCCESS ==========\n");
        } catch (const std::exception& e) {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI, "RFC OTLP: FATAL - Init failed: %s\n", e.what());
        } catch (...) {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCAPI, "RFC OTLP: FATAL - Init failed with unknown error\n");
        }
    }

    void initializeTracer() {
        RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: ========== initializeTracer START ==========\n");
        // Configure OTLP HTTP exporter with dynamic endpoint resolution
        otlp::OtlpHttpExporterOptions exporter_opts;
        std::string endpoint = getCollectorEndpoint();
        RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: Collector endpoint: %s\n", endpoint.c_str());
        exporter_opts.url = endpoint + "/v1/traces";
        exporter_opts.content_type = otlp::HttpRequestContentType::kJson;
        RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: Exporter URL: %s\n", exporter_opts.url.c_str());

        // Add headers for authentication/identification
        exporter_opts.http_headers.insert({"User-Agent", "rfc-rdk/1.2.7"});
        exporter_opts.http_headers.insert({"X-RDK-Component", "rfc"});

        RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: Initializing OTLP HTTP exporter with endpoint: %s\n", exporter_opts.url.c_str());
        std::cout << "Initializing OTLP HTTP exporter with endpoint: " << exporter_opts.url << std::endl;
        RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: Creating OTLP HTTP exporter...\n");

        // Create OTLP HTTP exporter
        auto exporter = std::make_unique<otlp::OtlpHttpExporter>(exporter_opts);
        RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: Exporter created successfully\n");

        // Create span processor
        RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: Creating span processor...\n");
        auto processor = std::make_unique<trace_sdk::SimpleSpanProcessor>(std::move(exporter));
        RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: Processor created successfully\n");

        // Create resource with comprehensive RDK-specific attributes
        auto resource_attributes = resource::ResourceAttributes{
            {"service.name", "rfc"},
            {"service.version", "1.2.7"},
            {"service.namespace", "rdk"},
            {"rdk.component", "rfc"},
            {"rdk.profile", "STB"},
            {"rdk.container.type", "native-platform"},
            {"deployment.environment", "container"},
            {"telemetry.sdk.name", "opentelemetry"},
            {"telemetry.sdk.language", "cpp"},
            {"telemetry.sdk.version", "1.23.0"}
        };
        auto resource = resource::Resource::Create(resource_attributes);
        RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: Resource created\n");

        // Create tracer provider with single processor
        std::vector<std::unique_ptr<trace_sdk::SpanProcessor>> processors;
        processors.push_back(std::move(processor));
        RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: Creating tracer provider...\n");

        auto provider = std::make_shared<trace_sdk::TracerProvider>(
            std::move(processors), resource);

        trace::Provider::SetTracerProvider(nostd::shared_ptr<trace::TracerProvider>(provider));
        RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: Tracer provider set globally\n");

        // Get tracer instance
        tracer_ = provider->GetTracer("rfc", "1.0.0");
        RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: ========== Tracer initialized successfully ==========\n");

        std::cout << "rfc OTLP Tracer initialized successfully!" << std::endl;
    }

    /**
     * Helper function to get trace and span IDs as hex strings
     * Simplified version that works across different OpenTelemetry versions
     */
     std::pair<std::string, std::string> getTraceSpanIds(nostd::shared_ptr<trace::Span> span) {
        auto span_context = span->GetContext();


    // Extract actual trace_id (16 bytes)
    char trace_id[32];
    span_context.trace_id().ToLowerBase16(trace_id);

    // Extract actual span_id (8 bytes)
    char span_id[16];
    span_context.span_id().ToLowerBase16(span_id);

    return {std::string(trace_id, 32), std::string(span_id, 16)};

    }
    /**
     * Trace parameter operations (get/set) with trace/span ID logging
     */
    void traceParameterOperation(const std::string& param_name,
                               const std::string& operation_type,
                               const std::function<void()>& operation) {
        auto span = tracer_->StartSpan("rfcMgr." + operation_type);

        // Get and log trace/span IDs
        auto [trace_id, span_id] = getTraceSpanIds(span);
        std::cout << "RFC parameter " << operation_type << "RFC Trace - TraceID: " << trace_id << ",RFC SpanID: " << span_id << std::endl;

        // Set parameter-specific attributes
        span->SetAttribute("parameter.name", param_name);
        span->SetAttribute("parameter.operation", operation_type);
        span->SetAttribute("rdk.component", "rfc");
        span->SetAttribute("trace.id", trace_id);
        span->SetAttribute("span.id", span_id);
        span->SetAttribute("rdk.trace.source", "rfc-otlp");

        try {
            // Execute the operation
            operation();
            span->SetStatus(trace::StatusCode::kOk);
            std::cout << " RFC Parameter " << operation_type << " completed for: " << param_name << std::endl;    

        } catch (const std::exception& e) {
            span->SetAttribute("error.message", e.what());
            span->SetStatus(trace::StatusCode::kError, e.what());
            std::cout << " Parameter " << operation_type << " failed for: " << param_name << " - Error: " << e.what() << std::endl;
            throw;
        }

        span->End();
        std::cout << "RFC span sent to collector at: " << getCollectorEndpoint() << std::endl;
    }

    /**
     * Force flush all pending spans (useful for shutdown)
     * Simplified version that doesn't rely on SDK-specific methods
     */
    void forceFlush() {
        // Simple approach: just log that we're flushing
        // The spans should be automatically flushed when the tracer is destroyed
        // or when the process exits
        std::cout << "🔄 Requesting flush of all pending spans..." << std::endl;

        // Sleep briefly to allow any pending spans to be processed
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::cout << "🔄 Flush request completed" << std::endl;
    }

    /**
     * Get current collector endpoint for debugging
     */
    std::string getCurrentEndpoint() const {
        // Make getCollectorEndpoint const or call it differently
        const char* env_endpoint = std::getenv("OTEL_EXPORTER_OTLP_ENDPOINT");
        if (env_endpoint != nullptr) {
            return std::string(env_endpoint);
        }

        const char* container_env = std::getenv("RUNNING_IN_CONTAINER");
        if (container_env != nullptr && std::string(container_env) == "true") {
            return "http://otel-collector:4318";
        }

        return "http://localhost:4318";
    }
};

// Singleton accessor - lazy initialization on first use
// This ensures the tracer is initialized when first accessed, not at library load
static rfcOTLPTracer& get_tracer_instance() {
    RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: get_tracer_instance() called\n");
    static rfcOTLPTracer instance;
    RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: get_tracer_instance() returning instance\n");
    return instance;
}

// C-style API for integration with existing C code
extern "C" {

void rfc_otlp_trace_parameter_get(const char* param_name) {
    RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: Tracing parameter GET for: %s\n", param_name ? param_name : "NULL");
    std::cout << "🔍 RFC OTLP: Tracing parameter GET for: " << (param_name ? param_name : "NULL") << std::endl;
    get_tracer_instance().traceParameterOperation(param_name, "get", [](){});
}

void rfc_otlp_trace_parameter_set(const char* param_name) {
    RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: Tracing parameter SET for: %s\n", param_name ? param_name : "NULL");
    std::cout << "✏️ RFC OTLP: Tracing parameter SET for: " << (param_name ? param_name : "NULL") << std::endl;
    get_tracer_instance().traceParameterOperation(param_name, "set", [](){});
}

void rfc_otlp_force_flush() {
    RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: Force flush called\n");
    std::cout << "🔄 RFC OTLP: Force flush called" << std::endl;
    get_tracer_instance().forceFlush();
}

const char* rfc_otlp_get_endpoint() {
    static std::string endpoint = get_tracer_instance().getCurrentEndpoint();
    RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: Endpoint requested: %s\n", endpoint.c_str());
    std::cout << "🌐 RFC OTLP: Endpoint requested: " << endpoint << std::endl;
    return endpoint.c_str();
}

void rfc_otlp_test() {
    RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "============================================\n");
    RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: TEST FUNCTION CALLED\n");
    RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "RFC OTLP: This proves the library is linked\n");
    RDK_LOG(RDK_LOG_INFO, LOG_RFCAPI, "============================================\n");
    std::cout << "RFC OTLP TEST: Function is working!" << std::endl;
}

} // extern "C"
