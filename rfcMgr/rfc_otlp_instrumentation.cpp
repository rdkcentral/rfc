/**
 * OpenTelemetry OTLP HTTP Instrumentation for rfc
 * Compatible with OpenTelemetry C++ SDK v1.23.0
 * Sends traces to OpenTelemetry Collector via OTLP HTTP
 * Supports distributed tracing with W3C Trace Context
 */
//trace libs
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/span_context.h>
#include <opentelemetry/context/propagation/global_propagator.h>
#include <opentelemetry/context/propagation/text_map_propagator.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>

//metrics libs
#include <opentelemetry/metrics/provider.h>
#include <opentelemetry/sdk/metrics/meter_provider.h>
#include <opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader.h>
#include <opentelemetry/exporters/otlp/otlp_http_metric_exporter.h>
#include <opentelemetry/sdk/metrics/view/view_registry.h>
#include <opentelemetry/sdk/metrics/aggregation/default_aggregation.h>

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <string>
#include <functional>
#include <sstream>
#include <iomanip>
#include <array>
#include <thread>
#include <chrono>
#include <map>
#include "../rfcapi/rfc_otlp_instrumentation.h"

#include "../rfcapi/rfc_otlp_instrumentation.h"

namespace trace = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace resource = opentelemetry::sdk::resource;
namespace nostd = opentelemetry::nostd;
namespace otlp = opentelemetry::exporter::otlp;
namespace context = opentelemetry::context;

namespace metrics_api = opentelemetry::metrics;
namespace metrics_sdk = opentelemetry::sdk::metrics;
namespace metrics_exporter = opentelemetry::exporter::otlp;

// Global storage for active span to maintain context across function calls
static nostd::shared_ptr<trace::Span> g_active_parent_span{};
static nostd::unique_ptr<context::Token> g_context_token{nullptr};
namespace metrics_sdk = opentelemetry::sdk::metrics;

// Simple carrier implementation for context propagation
class TextMapCarrier : public context::propagation::TextMapCarrier {
public:
    TextMapCarrier() = default;
    
    nostd::string_view Get(nostd::string_view key) const noexcept override {
        auto it = headers_.find(std::string(key));
        if (it != headers_.end()) {
            return nostd::string_view(it->second);
        }
        return "";
    }
    
    void Set(nostd::string_view key, nostd::string_view value) noexcept override {
        headers_[std::string(key)] = std::string(value);
    }
    
    const std::map<std::string, std::string>& GetHeaders() const {
        return headers_;
    }
    
private:
    std::map<std::string, std::string> headers_;
};

class rfcOTLPTracer {
private:
    nostd::shared_ptr<trace::Tracer> tracer_;
    nostd::shared_ptr<context::propagation::TextMapPropagator> propagator_;
    
    std::string getCollectorEndpoint() {
        // Check environment variable first for Docker container networking
        const char* env_endpoint = std::getenv("OTEL_EXPORTER_OTLP_ENDPOINT");
        if (env_endpoint != nullptr) {
            return std::string(env_endpoint);
        }
        
        // Check for container environment
        const char* container_env = std::getenv("RUNNING_IN_CONTAINER");
        if (container_env != nullptr && std::string(container_env) == "true") {
            // Use collector service name in Docker network
            return "http://10.0.0.182:4318";
        }
        
        // Default to localhost for non-container environments
        return "http://10.0.0.182:4318";
    }
    
public:
    rfcOTLPTracer() {
        initializeTracer();
    }
    
    void initializeTracer() {
        // Configure OTLP HTTP exporter with dynamic endpoint resolution
        otlp::OtlpHttpExporterOptions exporter_opts;
        exporter_opts.url = getCollectorEndpoint() + "/v1/traces";
        exporter_opts.content_type = otlp::HttpRequestContentType::kJson;
        
        std::cout << "Initializing RFC OTLP HTTP exporter with endpoint: " << exporter_opts.url << std::endl;
        
        // Create OTLP HTTP exporter
        auto exporter = std::make_unique<otlp::OtlpHttpExporter>(exporter_opts);
        
        // Create span processor
        auto processor = std::make_unique<trace_sdk::SimpleSpanProcessor>(std::move(exporter));
        
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
        
        // Create tracer provider with single processor
        std::vector<std::unique_ptr<trace_sdk::SpanProcessor>> processors;
        processors.push_back(std::move(processor));
        
        auto provider = std::make_shared<trace_sdk::TracerProvider>(
            std::move(processors), resource);
        
        trace::Provider::SetTracerProvider(nostd::shared_ptr<trace::TracerProvider>(provider));
        
        // Get tracer instance
        tracer_ = provider->GetTracer("rfc", "1.0.0");
        
        // Initialize W3C Trace Context propagator for distributed tracing
        propagator_ = nostd::shared_ptr<context::propagation::TextMapPropagator>(
            new trace::propagation::HttpTraceContext());
        context::propagation::GlobalTextMapPropagator::SetGlobalPropagator(propagator_);
        
        std::cout << "RFC OTLP Tracer initialized successfully with distributed tracing support!" << std::endl;
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
    
     /* Trace parameter operations (get/set) with trace/span ID logging
     */
    void traceParameterOperation(const std::string& param_name, 
                               const std::string& operation_type,
                               const std::function<void()>& operation) {
        auto span = tracer_->StartSpan("rfc.parameter." + operation_type);
        
        // Get and log trace/span IDs
        auto [trace_id, span_id] = getTraceSpanIds(span);
        std::cout << "[RFC] Parameter " << operation_type << " Trace - TraceID: " << trace_id << ", SpanID: " << span_id << std::endl;
        
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
            std::cout << "[RFC] Parameter " << operation_type << " completed for: " << param_name << std::endl;
            
        } catch (const std::exception& e) {
            span->SetAttribute("error.message", e.what());
            span->SetStatus(trace::StatusCode::kError, e.what());
            std::cout << "[RFC] Parameter " << operation_type << " failed for: " << param_name << " - Error: " << e.what() << std::endl;
            throw;
        }
        
        span->End();
        std::cout << "[RFC] Span sent to collector at: " << getCollectorEndpoint() << std::endl;
    }
    
    /**
     * Start a distributed trace and return span with context for propagation
     */
    nostd::shared_ptr<trace::Span> startDistributedTrace(const std::string& param_name,
                                                          const std::string& operation_type,
                                                          rfc_trace_context_t* context_out) {
        // Create span options for root span
        trace::StartSpanOptions options;
        options.kind = trace::SpanKind::kClient;  // RFC is the client calling tr69hostif
        
        auto span = tracer_->StartSpan("rfc.tr181." + operation_type, options);
        
        // Get trace and span IDs
        auto [trace_id, span_id] = getTraceSpanIds(span);
        
        // Fill output context structure
        if (context_out) {
            strncpy(context_out->trace_id, trace_id.c_str(), 32);
            context_out->trace_id[32] = '\0';
            strncpy(context_out->span_id, span_id.c_str(), 16);
            context_out->span_id[16] = '\0';
            
            // Set trace flags (sampled)
            auto span_context = span->GetContext();
            snprintf(context_out->trace_flags, 3, "%02x", span_context.trace_flags().flags());
        }
        
        // Set span attributes
        span->SetAttribute("parameter.name", param_name);
        span->SetAttribute("parameter.operation", operation_type);
        span->SetAttribute("rdk.component", "rfc");
        span->SetAttribute("rdk.service", "rfc");
        span->SetAttribute("rdk.call.destination", "tr69hostif");
        span->SetAttribute("span.kind", "client");
        
        // CRITICAL: Set this span as the active span in the current context
        // This allows child spans to find and link to this parent span
        auto current_context = context::RuntimeContext::GetCurrent();
        auto new_context = trace::SetSpan(current_context, span);
        g_context_token = context::RuntimeContext::Attach(new_context);
        
        // Store span globally for child span access
        g_active_parent_span = span;
        
        std::cout << "[RFC] Started distributed trace: " << operation_type 
                  << " | TraceID: " << trace_id << " | SpanID: " << span_id << std::endl;
        std::cout << "[RFC] Parent span set as active in context and stored globally" << std::endl;
        
        return span;
    }
    
    /**
     * End a span with status
     */
    void endSpan(nostd::shared_ptr<trace::Span> span, bool success, const char* error_msg) {
        if (!span) return;
        
        if (success) {
            span->SetStatus(trace::StatusCode::kOk);
        } else {
            span->SetStatus(trace::StatusCode::kError, error_msg ? error_msg : "Unknown error");
            if (error_msg) {
                span->SetAttribute("error.message", error_msg);
            }
        }
        
        span->End();
        std::cout << "[RFC] Span ended with status: " << (success ? "OK" : "ERROR") << std::endl;
    }
    
    /**
     * Force flush all pending spans (useful for shutdown)
     */
    void forceFlush() {
        std::cout << "🔄 Requesting flush of all pending spans..." << std::endl;
        
        // Sleep briefly to allow any pending spans to be processed
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "🔄 Flush request completed" << std::endl;
    }
    
    /**
     * Get the tracer instance for creating child spans
     */
    nostd::shared_ptr<trace::Tracer> getTracer() {
        return tracer_;
    }

    /**
     * Get current collector endpoint for debugging
     */
    std::string getCurrentEndpoint() const {
        // Make getCollectorEndpoint const or call it differently
        const char* env_endpoint = std::getenv("OTEL_EXPORTER_OTLP_ENDPOINT");
        if (env_endpoint != nullptr) {
            return "http://10.0.0.182:4318";
        }
        
        const char* container_env = std::getenv("RUNNING_IN_CONTAINER");
        if (container_env != nullptr && std::string(container_env) == "true") {
            return "http://10.0.0.182:4318";
        }
        
        return "http://10.0.0.182:4318";
    }
};

class rfcMetricsCollector {
private:
    opentelemetry::nostd::shared_ptr<metrics_api::Meter> meter_;
    
    // Counter metrics
    std::unique_ptr<metrics_api::Counter<uint64_t>> parameter_operations_total_;
    
    // Histogram metrics
    std::unique_ptr<metrics_api::Histogram<double>> parameter_operation_duration_;

public:
    rfcMetricsCollector() {
        initializeMetrics();
    }
    
    void initializeMetrics() {
        try {
            // Configure OTLP HTTP metric exporter
            otlp::OtlpHttpMetricExporterOptions exporter_opts;
            
            const char* env_endpoint = std::getenv("OTEL_EXPORTER_OTLP_ENDPOINT");
            std::string base_endpoint = env_endpoint ? env_endpoint : "http://localhost:4318";
            const char* container_env = std::getenv("RUNNING_IN_CONTAINER");
            if (!env_endpoint && container_env != nullptr && std::string(container_env) == "true") {
                base_endpoint = "http://10.0.0.182:4318";
            }
            
            exporter_opts.url = base_endpoint + "/v1/metrics";
            exporter_opts.content_type = otlp::HttpRequestContentType::kJson;
            exporter_opts.timeout = std::chrono::seconds(10);
            
            std::cout << "Initializing OTLP HTTP metrics exporter with endpoint: " 
                      << exporter_opts.url << std::endl;
            
            std::unique_ptr<otlp::OtlpHttpMetricExporter> exporter(
                new otlp::OtlpHttpMetricExporter(exporter_opts));
            
            metrics_sdk::PeriodicExportingMetricReaderOptions reader_options;
            reader_options.export_interval_millis = std::chrono::milliseconds(10000);
            reader_options.export_timeout_millis = std::chrono::milliseconds(5000);
            
            std::unique_ptr<metrics_sdk::PeriodicExportingMetricReader> reader(
                new metrics_sdk::PeriodicExportingMetricReader(std::move(exporter), reader_options));
            
            auto resource_attributes = resource::ResourceAttributes{
                {"service.name", "rfc"},
                {"service.version", "1.2.7"},
                {"service.namespace", "rdk"},
                {"rdk.component", "rfc"},
                {"rdk.profile", "STB"},
                {"deployment.environment", "container"}
            };
            auto resource = resource::Resource::Create(resource_attributes);
            
            std::unique_ptr<metrics_sdk::ViewRegistry> view_registry(new metrics_sdk::ViewRegistry());
            auto provider = opentelemetry::nostd::shared_ptr<metrics_sdk::MeterProvider>(
                new metrics_sdk::MeterProvider(std::move(view_registry), resource)
            );
            
            provider->AddMetricReader(std::move(reader));
            
            metrics_api::Provider::SetMeterProvider(std::move(provider));
            
            meter_ = metrics_api::Provider::GetMeterProvider()->GetMeter("rfc", "1.0.0");
            
            parameter_operations_total_ = meter_->CreateUInt64Counter(
                "rfc.parameter.operations.total",
                "Total number of parameter operations (get/set)",
                "operations");
            
            parameter_operation_duration_ = meter_->CreateDoubleHistogram(
                "rfc.parameter.operation.duration",
                "Parameter operation processing duration",
                "seconds");
            
            std::cout <<"Metrics will be exported every 10 seconds" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "❌ Failed to initialize metrics: " << e.what() << std::endl;
            throw;
        }
    }
    
    void recordParameterOperation(const std::string& param_name, 
                                   const std::string& operation_type,
                                   double duration_seconds) {
        parameter_operations_total_->Add(1, {
            {"parameter.name", param_name},
            {"operation.type", operation_type}
        });
        
        auto context = opentelemetry::context::Context{};
        parameter_operation_duration_->Record(duration_seconds, {
            {"parameter.name", param_name},
            {"operation.type", operation_type}
        }, context);
        
        std::cout << "📊 Parameter Operation Metric: " << operation_type << " " 
                  << param_name << " (" << duration_seconds << "s)" << std::endl;
    }
};

// Global metrics collector
static std::unique_ptr<rfcMetricsCollector> g_metrics_collector;

void initializeMetricsCollector() {
    if (!g_metrics_collector) {
        g_metrics_collector.reset(new rfcMetricsCollector());
    }
}

// Global tracer instance
static rfcOTLPTracer g_otlp_tracer;

// C-style API for integration with existing C code
extern "C" {

void rfc_metrics_init() {
    initializeMetricsCollector();
}

void rfc_metrics_record_parameter_operation(
    const char* param_name, const char* operation_type, double duration_seconds) {
    if (g_metrics_collector) {
        g_metrics_collector->recordParameterOperation(param_name, operation_type, duration_seconds);
    }
}

// New distributed tracing API
void* rfc_otlp_start_parameter_get(const char* param_name, rfc_trace_context_t* context) {
    if (!param_name || !context) {
        return nullptr;
    }
    
    try {
        auto span = g_otlp_tracer.startDistributedTrace(param_name, "get", context);
        // Return raw pointer - caller must call rfc_otlp_end_span to clean up
        return new nostd::shared_ptr<trace::Span>(span);
    } catch (const std::exception& e) {
        std::cerr << "[RFC] Failed to start trace: " << e.what() << std::endl;
        return nullptr;
    }
}

void* rfc_otlp_start_parameter_set(const char* param_name, rfc_trace_context_t* context) {
    if (!param_name || !context) {
        std::cout << "[RFC] Invalid parameters for SET operation" << std::endl;
        return nullptr;
    }
    
    try {
        auto span = g_otlp_tracer.startDistributedTrace(param_name, "set", context);
        if (span) {
            std::cout << "[RFC] SET parameter span created and context activated for: " << param_name << std::endl;
            // Return raw pointer - caller must call rfc_otlp_end_span to clean up
            return new nostd::shared_ptr<trace::Span>(span);
        }
    } catch (const std::exception& e) {
        std::cerr << "[RFC] Failed to start trace: " << e.what() << std::endl;
    }
    
    return nullptr;
}

void rfc_otlp_end_span(void* span_handle, bool success, const char* error_msg) {
    if (!span_handle) {
        return;
    }
    
    try {
        auto* span_ptr = static_cast<nostd::shared_ptr<trace::Span>*>(span_handle);
        
        // Check if this is the global parent span being ended
        if (g_active_parent_span && *span_ptr == g_active_parent_span) {
            std::cout << "[RFC] Ending global parent span, cleaning up context" << std::endl;
            
            // Clean up global state
            g_active_parent_span = nullptr;
            if (g_context_token) {
                // Token will automatically detach when reset
                g_context_token.reset();
            }
        }
        
        g_otlp_tracer.endSpan(*span_ptr, success, error_msg);
        delete span_ptr;
    } catch (const std::exception& e) {
        std::cerr << "[RFC] Failed to end span: " << e.what() << std::endl;
    }
}

// Attribute setting functions
void rfc_otlp_set_span_attribute_string(void* span_handle, const char* key, const char* value) {
    if (!span_handle || !key || !value) {
        return;
    }
    
    try {
        auto* span_ptr = static_cast<nostd::shared_ptr<trace::Span>*>(span_handle);
        if (*span_ptr) {
            (*span_ptr)->SetAttribute(key, value);
        }
    } catch (const std::exception& e) {
        std::cerr << "[RFC] Failed to set span string attribute: " << e.what() << std::endl;
    }
}

void rfc_otlp_set_span_attribute_int(void* span_handle, const char* key, long value) {
    if (!span_handle || !key) {
        return;
    }
    
    try {
        auto* span_ptr = static_cast<nostd::shared_ptr<trace::Span>*>(span_handle);
        if (*span_ptr) {
            (*span_ptr)->SetAttribute(key, static_cast<int64_t>(value));
        }
    } catch (const std::exception& e) {
        std::cerr << "[RFC] Failed to set span integer attribute: " << e.what() << std::endl;
    }
}

void rfc_otlp_set_span_attribute_double(void* span_handle, const char* key, double value) {
    if (!span_handle || !key) {
        return;
    }
    
    try {
        auto* span_ptr = static_cast<nostd::shared_ptr<trace::Span>*>(span_handle);
        if (*span_ptr) {
            (*span_ptr)->SetAttribute(key, value);
        }
    } catch (const std::exception& e) {
        std::cerr << "[RFC] Failed to set span double attribute: " << e.what() << std::endl;
    }
}

void rfc_otlp_set_span_attribute_bool(void* span_handle, const char* key, int value) {
    if (!span_handle || !key) {
        return;
    }
    
    try {
        auto* span_ptr = static_cast<nostd::shared_ptr<trace::Span>*>(span_handle);
        if (*span_ptr) {
            (*span_ptr)->SetAttribute(key, value != 0);
        }
    } catch (const std::exception& e) {
        std::cerr << "[RFC] Failed to set span boolean attribute: " << e.what() << std::endl;
    }
}

void rfc_otlp_add_span_event(void* span_handle, const char* event_name) {
    if (!span_handle || !event_name) {
        return;
    }
    
    try {
        auto* span_ptr = static_cast<nostd::shared_ptr<trace::Span>*>(span_handle);
        if (*span_ptr) {
            (*span_ptr)->AddEvent(event_name);
        }
    } catch (const std::exception& e) {
        std::cerr << "[RFC] Failed to add span event: " << e.what() << std::endl;
    }
}

/**
 * Start a child span for an HTTP operation within a parent span
 * Creates a child span linked to the current parent span context
 */
void* rfc_otlp_start_child_span(const char* span_name, const char* parent_span_id) {
    if (!span_name) {
        std::cout << "[RFC] Child span creation failed: span_name is NULL" << std::endl;
        return nullptr;
    }
    
    std::cout << "[RFC] Starting child span: " << span_name << std::endl;
    
    try {
        // Get current tracer to create child span
        auto tracer = g_otlp_tracer.getTracer();
        if (!tracer) {
            std::cout << "[RFC] Failed to get tracer for child span" << std::endl;
            return nullptr;
        }
        
        std::cout << "[RFC] Tracer obtained, creating child span" << std::endl;
        
        // Check for globally stored parent span first
        nostd::shared_ptr<trace::Span> parent_span = g_active_parent_span;
        
        if (!parent_span) {
            // Fallback: try to get current active span
            auto current_context = context::RuntimeContext::GetCurrent();
            parent_span = trace::GetSpan(current_context);
        }
        
        if (!parent_span || !parent_span->GetContext().IsValid()) {
            std::cout << "[RFC] Warning: No active parent span found, creating root span" << std::endl;
        } else {
            auto parent_context = parent_span->GetContext();
            char parent_trace_id[32];
            parent_context.trace_id().ToLowerBase16(nostd::span<char, 32>(parent_trace_id, 32));
            std::cout << "[RFC] Found active parent span with trace ID: " << std::string(parent_trace_id, 32) << std::endl;
        }
        
        // Create child span options with explicit parent context
        trace::StartSpanOptions options;
        options.kind = trace::SpanKind::kClient;
        
        if (parent_span && parent_span->GetContext().IsValid()) {
            // Create context with parent span
            auto parent_context = context::RuntimeContext::GetCurrent();
            auto span_context = trace::SetSpan(parent_context, parent_span);
            options.parent = span_context;
        }
        
        // Create child span
        auto span = tracer->StartSpan(span_name, options);
        if (span) {
            // Get span IDs for logging
            auto span_context = span->GetContext();
            char trace_id[32];
            char span_id[16];
            span_context.trace_id().ToLowerBase16(nostd::span<char, 32>(trace_id, 32));
            span_context.span_id().ToLowerBase16(nostd::span<char, 16>(span_id, 16));
            
            std::cout << "[RFC] Child span created successfully: " << span_name 
                      << " | TraceID: " << std::string(trace_id, 32) 
                      << " | SpanID: " << std::string(span_id, 16) << std::endl;
            
            // Allocate and store the span pointer
            auto* span_ptr = new nostd::shared_ptr<trace::Span>(span);
            return span_ptr;
        } else {
            std::cout << "[RFC] Failed to create child span: span is null" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[RFC] Exception in child span creation: " << e.what() << std::endl;
    }
    sleep(2); 
    return nullptr;
}

/**
 * End a child span
 */
void rfc_otlp_end_child_span(void* span_handle, bool success, const char* error_msg) {
    if (!span_handle) {
        std::cout << "[RFC] Cannot end child span: span_handle is NULL" << std::endl;
        return;
    }
    
    std::cout << "[RFC] Ending child span with status: " << (success ? "success" : "failed") << std::endl;
    
    try {
        auto* span_ptr = static_cast<nostd::shared_ptr<trace::Span>*>(span_handle);
        if (*span_ptr) {
            // Add status attribute
            (*span_ptr)->SetAttribute("operation.status", success ? "success" : "failed");
            
            // Add error event if failure
            if (!success && error_msg) {
                (*span_ptr)->AddEvent("operation.error");
                (*span_ptr)->SetAttribute("error.message", error_msg);
                std::cout << "[RFC] Added error to child span: " << error_msg << std::endl;
            }
            
            // End the span
            (*span_ptr)->End();
            std::cout << "[RFC] Child span ended successfully" << std::endl;
        } else {
            std::cout << "[RFC] Cannot end child span: span pointer is null" << std::endl;
        }
        
        // Clean up allocated memory
        delete span_ptr;
    } catch (const std::exception& e) {
        std::cout << "[RFC] Exception in child span end: " << e.what() << std::endl;
    }
}

/**
 * Construct W3C Trace Context header value for HTTP propagation
 * Format: "00-trace_id-span_id-trace_flags"
 */
int rfc_otlp_get_trace_header(rfc_trace_context_t* context, char* header_buffer, int buffer_size) {
    if (!context || !header_buffer || buffer_size < 60) {
        return -1;
    }
    
    try {
        // Format: 00-<trace_id>-<span_id>-<trace_flags>
        int written = snprintf(header_buffer, buffer_size, "00-%s-%s-%s",
                              context->trace_id,
                              context->span_id,
                              context->trace_flags);
        
        if (written < 0 || written >= buffer_size) {
            std::cerr << "[RFC] Trace header buffer too small" << std::endl;
            return -1;
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[RFC] Failed to construct trace header: " << e.what() << std::endl;
        return -1;
    }
}

/**
 * Get current active span's trace context for HTTP header propagation
 * This extracts the trace context from the current active span
 */
int rfc_otlp_get_current_trace_header(char* header_buffer, int buffer_size) {
    if (!header_buffer || buffer_size < 60) {
        std::cout << "[RFC] Invalid buffer for trace header" << std::endl;
        return -1;
    }
    
    try {
        // Check for globally stored parent span first
        nostd::shared_ptr<trace::Span> span = g_active_parent_span;
        
        if (!span) {
            // Fallback: get current active span context using OpenTelemetry's runtime context
            auto current_context = context::RuntimeContext::GetCurrent();
            span = trace::GetSpan(current_context);
        }
        
        if (!span || !span->GetContext().IsValid()) {
            std::cout << "[RFC] No active span found for trace header extraction" << std::endl;
            return -1;
        }
        
        auto span_context = span->GetContext();
        
        // Extract trace ID (16 bytes -> 32 hex chars)
        char trace_id[32];  // 32 chars, no null terminator
        span_context.trace_id().ToLowerBase16(nostd::span<char, 32>(trace_id, 32));
        
        // Extract span ID (8 bytes -> 16 hex chars)
        char span_id[16];   // 16 chars, no null terminator
        span_context.span_id().ToLowerBase16(nostd::span<char, 16>(span_id, 16));
        
        // Extract trace flags (1 byte -> 2 hex chars)
        char flags[3];
        snprintf(flags, 3, "%02x", span_context.trace_flags().flags());
        
        // Format W3C traceparent header: version-traceid-spanid-flags
        int written = snprintf(header_buffer, buffer_size, "00-%.*s-%.*s-%s",
                              32, trace_id, 16, span_id, flags);
        
        if (written < 0 || written >= buffer_size) {
            std::cout << "[RFC] Trace header buffer too small" << std::endl;
            return -1;
        }
        
        std::cout << "[RFC] Generated traceparent header: " << header_buffer << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "[RFC] Exception extracting trace header: " << e.what() << std::endl;
        return -1;
    }
}

// Legacy API (for backward compatibility)
void rfc_otlp_trace_parameter_get(const char* param_name) {
    g_otlp_tracer.traceParameterOperation(param_name, "get", [](){});
}

void rfc_otlp_trace_parameter_set(const char* param_name) {
    g_otlp_tracer.traceParameterOperation(param_name, "set", [](){});
}

void rfc_otlp_force_flush() {
    g_otlp_tracer.forceFlush();
}

const char* rfc_otlp_get_endpoint() {
    static std::string endpoint = g_otlp_tracer.getCurrentEndpoint();
    return endpoint.c_str();
}

}
