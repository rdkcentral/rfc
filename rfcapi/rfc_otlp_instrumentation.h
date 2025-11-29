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
void rfc_otlp_test(void);

#ifdef __cplusplus
}
#endif

#endif // RFC_OTLP_INSTRUMENTATION_H
