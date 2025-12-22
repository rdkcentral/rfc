/**
 * OpenTelemetry OTLP HTTP Instrumentation Header for RFC
 * C-compatible interface for tracing RFC operations with distributed tracing support
 */

#ifndef RFC_OTLP_INSTRUMENTATION_H
#define RFC_OTLP_INSTRUMENTATION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Trace context structure for distributed tracing
 * Contains W3C Trace Context fields for propagation
 */
typedef struct {
    char trace_id[33];     // 32 hex chars + null terminator (128-bit trace ID)
    char span_id[17];      // 16 hex chars + null terminator (64-bit span ID)
    char trace_flags[3];   // 2 hex chars + null terminator (8-bit flags)
} rfc_trace_context_t;

/**
 * Start a distributed trace for RFC parameter GET operation
 * Creates a parent span and returns trace context for propagation
 * @param param_name The parameter name being retrieved
 * @param context Output parameter for trace context (allocated by caller)
 * @return Span handle (void*) to be passed to rfc_otlp_end_span, or NULL on error
 */
void* rfc_otlp_start_parameter_get(const char* param_name, rfc_trace_context_t* context);

/**
 * Start a distributed trace for RFC parameter SET operation
 * Creates a parent span and returns trace context for propagation
 * @param param_name The parameter name being set
 * @param context Output parameter for trace context (allocated by caller)
 * @return Span handle (void*) to be passed to rfc_otlp_end_span, or NULL on error
 */
void* rfc_otlp_start_parameter_set(const char* param_name, rfc_trace_context_t* context);

/**
 * End a distributed trace span
 * @param span_handle The span handle returned from rfc_otlp_start_parameter_*
 * @param success true if operation succeeded, false otherwise
 * @param error_msg Error message if success is false, NULL otherwise
 */
void rfc_otlp_end_span(void* span_handle, bool success, const char* error_msg);

/**
 * Set a string attribute on a span
 * @param span_handle The span handle returned from rfc_otlp_start_parameter_*
 * @param key The attribute key name
 * @param value The attribute string value
 */
void rfc_otlp_set_span_attribute_string(void* span_handle, const char* key, const char* value);

/**
 * Set an integer attribute on a span
 * @param span_handle The span handle returned from rfc_otlp_start_parameter_*
 * @param key The attribute key name
 * @param value The attribute integer value
 */
void rfc_otlp_set_span_attribute_int(void* span_handle, const char* key, long value);

/**
 * Set a double attribute on a span
 * @param span_handle The span handle returned from rfc_otlp_start_parameter_*
 * @param key The attribute key name
 * @param value The attribute double value
 */
void rfc_otlp_set_span_attribute_double(void* span_handle, const char* key, double value);

/**
 * Set a boolean attribute on a span
 * @param span_handle The span handle returned from rfc_otlp_start_parameter_*
 * @param key The attribute key name
 * @param value The attribute boolean value (0 = false, 1 = true)
 */
void rfc_otlp_set_span_attribute_bool(void* span_handle, const char* key, int value);

/**
 * Add an event to a span
 * @param span_handle The span handle returned from rfc_otlp_start_parameter_*
 * @param event_name The name of the event
 */
void rfc_otlp_add_span_event(void* span_handle, const char* event_name);

/**
 * Start a child span for an HTTP operation within a parent span
 * Creates a child span linked to the current parent span context
 * @param span_name The name of the child span (e.g., "http.request", "tr69.setParameter")
 * @param parent_span_id The parent span ID for linking (can be NULL to use current context)
 * @return Span handle (void*) to be passed to rfc_otlp_end_child_span, or NULL on error
 */
void* rfc_otlp_start_child_span(const char* span_name, const char* parent_span_id);

/**
 * End a child span
 * @param span_handle The span handle returned from rfc_otlp_start_child_span
 * @param success true if operation succeeded, false otherwise
 * @param error_msg Error message if success is false, NULL otherwise
 */
void rfc_otlp_end_child_span(void* span_handle, bool success, const char* error_msg);

/**
 * Construct W3C Trace Context header value for HTTP propagation
 * @param context The trace context to convert
 * @param header_buffer Buffer to store the "traceparent" header value (must be at least 100 bytes)
 * @param buffer_size Size of the buffer
 * @return 0 on success, -1 on error
 */
int rfc_otlp_get_trace_header(rfc_trace_context_t* context, char* header_buffer, int buffer_size);

/**
 * Get current active span's trace context for HTTP header propagation
 * Extracts the W3C traceparent header from the currently active span
 * @param header_buffer Buffer to store the "traceparent" header value (must be at least 60 bytes)
 * @param buffer_size Size of the buffer
 * @return 0 on success, -1 on error
 */
int rfc_otlp_get_current_trace_header(char* header_buffer, int buffer_size);

/**
 * Legacy API - Trace an RFC parameter GET operation (non-distributed)
 * @param param_name The parameter name being retrieved
 */
void rfc_otlp_trace_parameter_get(const char* param_name);

/**
 * Legacy API - Trace an RFC parameter SET operation (non-distributed)
 * @param param_name The parameter name being set
 */
void rfc_otlp_trace_parameter_set(const char* param_name);

/**
 * Force flush all pending spans to the collector
 */
void rfc_otlp_force_flush(void);

/**
 * Get the current OTLP collector endpoint
 * @return The endpoint URL string
 */
const char* rfc_otlp_get_endpoint(void);

/**
 * Test function to verify OTLP instrumentation is linked
 */
void rfc_metrics_init();

void rfc_metrics_record_parameter_operation(const char* param_name, const char* operation_type, double duration_seconds);

#ifdef __cplusplus
}
#endif

#endif // RFC_OTLP_INSTRUMENTATION_H
