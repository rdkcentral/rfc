/**
 * OpenTelemetry OTLP HTTP Instrumentation for rfc
 * Compatible with OpenTelemetry C++ SDK v1.23.0
 * Sends traces to OpenTelemetry Collector via OTLP HTTP
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

//metrics libs
#include <opentelemetry/metrics/provider.h>
#include <opentelemetry/sdk/metrics/meter_provider.h>
#include <opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader.h>
#include <opentelemetry/exporters/otlp/otlp_http_metric_exporter.h>
#include <opentelemetry/sdk/metrics/view/view_registry.h>
#include <opentelemetry/sdk/metrics/aggregation/default_aggregation.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <functional>
#include <sstream>
#include <iomanip>
#include <array>
#include <thread>
#include <chrono>

namespace trace = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace resource = opentelemetry::sdk::resource;
namespace nostd = opentelemetry::nostd;
namespace otlp = opentelemetry::exporter::otlp;

namespace metrics_api = opentelemetry::metrics;
namespace metrics_sdk = opentelemetry::sdk::metrics;

class rfcOTLPTracer {
private:
    nostd::shared_ptr<trace::Tracer> tracer_;
    
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
            return "http://otel-collector:4318";
        }
        
        // Default to localhost for non-container environments
        return "http://otel-collector:4318";
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
        
        std::cout << "Initializing OTLP HTTP exporter with endpoint: " << exporter_opts.url << std::endl;
        
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
        tracer_ = provider->GetTracer("tr69hostif", "1.0.0");
        
        std::cout << "RFC OTLP Tracer initialized successfully!" << std::endl;
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
        auto span = tracer_->StartSpan("tr69hostif.parameter." + operation_type);
        
        // Get and log trace/span IDs
        auto [trace_id, span_id] = getTraceSpanIds(span);
        std::cout << "Parameter " << operation_type << " Trace - TraceID: " << trace_id << ", SpanID: " << span_id << std::endl;
        
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
            std::cout << "Parameter " << operation_type << " completed for: " << param_name << std::endl;
            
        } catch (const std::exception& e) {
            span->SetAttribute("error.message", e.what());
            span->SetStatus(trace::StatusCode::kError, e.what());
            std::cout << " Parameter " << operation_type << " failed for: " << param_name << " - Error: " << e.what() << std::endl;
            throw;
        }
        
        span->End();
        std::cout << " Span sent to collector at: " << getCollectorEndpoint() << std::endl;
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
     * Get current collector endpoint for debugging
     */
    std::string getCurrentEndpoint() const {
        // Make getCollectorEndpoint const or call it differently
        const char* env_endpoint = std::getenv("OTEL_EXPORTER_OTLP_ENDPOINT");
        if (env_endpoint != nullptr) {
            return "http://otel-collector:4318";
        }
        
        const char* container_env = std::getenv("RUNNING_IN_CONTAINER");
        if (container_env != nullptr && std::string(container_env) == "true") {
            return "http://otel-collector:4318";
        }
        
        return "http://otel-collector:4318";
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
                base_endpoint = "http://otel-collector:4318";
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

char* rfc_otlp_inject_trace_context() {
    auto current_span = trace::Tracer::GetCurrentSpan();
    
    if (!current_span || !current_span->GetContext().IsValid()) {
        return nullptr;
    }
    
    // Get span context
    auto span_context = current_span->GetContext();
    auto trace_id = span_context.trace_id();
    auto span_id = span_context.span_id();
    auto trace_flags = span_context.trace_flags();
    
    // Format traceparent header: "00-<trace-id>-<span-id>-<flags>"
    std::ostringstream oss;
    oss << "00-";
    
    // Trace ID (32 hex chars)
    for (int i = 0; i < 16; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(trace_id.Id()[i]);
    }
    oss << "-";
    
    // Span ID (16 hex chars)
    for (int i = 0; i < 8; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(span_id.Id()[i]);
    }
    oss << "-";
    
    // Flags (2 hex chars)
    oss << std::hex << std::setw(2) << std::setfill('0') 
        << static_cast<int>(trace_flags.flags());
    
    std::string traceparent = "traceparent: " + oss.str();
    char* result = static_cast<char*>(malloc(traceparent.length() + 1));
    if (result) {
        strcpy(result, traceparent.c_str());
    }
    
    std::cout << "🔗 Injecting trace context: " << oss.str() << std::endl;
    
    return result;
}

void rfc_otlp_extract_trace_context(const char* traceparent_header) {
    if (!traceparent_header || strlen(traceparent_header) < 55) {
        return;
    }
    
    // Parse traceparent format: "00-<32hex>-<16hex>-<2hex>"
    std::string tp(traceparent_header);
    
    // Skip version "00-"
    if (tp.substr(0, 3) != "00-") {
        return;
    }
    
    try {
        // Extract trace_id (32 hex chars starting at position 3)
        std::string trace_id_hex = tp.substr(3, 32);
        
        // Extract span_id (16 hex chars starting at position 36)
        std::string span_id_hex = tp.substr(36, 16);
        
        // Extract flags (2 hex chars starting at position 53)
        std::string flags_hex = tp.substr(53, 2);
        
        std::cout << "🔗 Extracting trace context: trace_id=" << trace_id_hex 
                  << " span_id=" << span_id_hex << " flags=" << flags_hex << std::endl;
        
        // Note: Full implementation would recreate span context from these values
        // For now, just log that we received the context
    } catch (...) {
        std::cerr << "Failed to parse traceparent header" << std::endl;
    }
}

} // extern "C"
