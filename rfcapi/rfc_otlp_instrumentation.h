/**
 * OpenTelemetry OTLP HTTP Instrumentation Header for RFC
 * C-compatible interface for tracing RFC operations
 */

#ifndef RFC_OTLP_INSTRUMENTATION_H
#define RFC_OTLP_INSTRUMENTATION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Trace an RFC parameter GET operation
 * @param param_name The parameter name being retrieved
 */
void rfc_otlp_trace_parameter_get(const char* param_name);

/**
 * Trace an RFC parameter SET operation
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

/**
 * Inject trace context into HTTP headers for distributed tracing
 * Returns a malloc'd string with "traceparent: XX-XXXX-XXXX-XX" format
 * Caller must free the returned string
 * @return HTTP header string or NULL if no active span
 */
char* rfc_otlp_inject_trace_context(void);

/**
 * Extract and activate trace context from incoming HTTP traceparent header
 * @param traceparent_header The traceparent header value (format: "00-traceid-spanid-flags")
 */
void rfc_otlp_extract_trace_context(const char* traceparent_header);

#ifdef __cplusplus
}
#endif

#endif // RFC_OTLP_INSTRUMENTATION_H
