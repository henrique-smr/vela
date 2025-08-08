#version 330 core

// Input from vertex shader
in vec2 fragTexCoord;

// Output
out vec4 finalColor;

// Uniforms
uniform sampler2D u_audio_channel_0;
uniform sampler2D u_audio_channel_1;
uniform sampler2D u_spectrum_channel_0;
uniform sampler2D u_spectrum_channel_1;
uniform vec2 u_resolution;
uniform int u_buffer_size;
uniform float u_signal;
uniform float u_time;

float avgFreq(float start, float end, float step) {
    float div = 0.0;
    float total = 0.0;
    for (float pos = start; pos < end; pos += step) {
        div += 1.0;
        total += texture(u_audio_channel_0, vec2(pos)).x;
    }
    return total / div;
}

vec3 palette( in float t ) {
	float bassFreq = pow(avgFreq(0.0, 0.1, 0.01), 0.85);
	float medFreq = pow(avgFreq(0.1, 0.6, 0.01), 0.85);
	float topFreq = pow(avgFreq(0.6, 1.0, 0.01), 0.85);
	vec3 a = vec3(0.5, 0.5, 0.5);
	vec3 b = vec3(0.5, 0.5, 0.5);
	vec3 c = vec3(2.0, 1.0, medFreq);
	vec3 d = vec3(0.50, 0.20, 0.25);
	return a + b*cos( 6.283185*(c*t+d) );
}

mat2 rot2D(float angle) {
	float s = sin(angle);
	float c = cos(angle);
	return mat2(c, -s, s, c);
}
float sdSphere(vec3 p, float r) {
	return length(p) - r; // distance to a sphere of radius 1
}

// Scene distance
float map(vec3 p) {
	vec3 q = p;

	q.z += fract(u_time*1.);

	//Space repeatition
	q.xy = fract(q.xy) - .5;
	q.z = mod(q.z, .5) - .25;

	vec3 spherePos = vec3(0.,0.,0.);
	float sphere = sdSphere(q - spherePos, .2);
	return sphere; // distance to a sphere of radius 1
}


void main() {

	// vec2 uv = (fragCoord * 2. - iResolution.xy) / iResolution.y;
	vec2 uv = fragTexCoord.xy * 2.0 - 1.0; // Normalized coordinates from -1 to 1
	uv.x *= u_resolution.x / u_resolution.y; // Adjust for aspect ratio

	// vec2 m = (iMouse.xy*2. - iResolution.xy) / iResolution.y;

	// Initialization
	vec3 ro = vec3(0, 0, -7);         // ray origin
	vec3 rd = normalize(vec3(uv*1.5, 1)); // ray direction
	vec3 col = vec3(0);               // final pixel color

	float t = 0.; // total distance travelled

	//ro.yz *= rot2D(-m.y*2.);
	// rd.yz *= rot2D(-m.y*2.);
	// ro.xz *= rot2D(-m.x*2.);
	// rd.xz *= rot2D(-m.x*2.);

	// Raymarching
	int i;
	for (i = 0; i < 80; i++) {
		vec3 p = ro + rd * t;     // position along the ray

		p.xy *= rot2D(t * 0.2); // Rotate the scene based on audio signal and time
		p.y += sin(u_time*4. +  t*2. ) * 0.15; // Add some animation to the scene

		float d = map(p);         // current distance to the scene

		t += d;                   // "march" the ray

		if (d < .001) break;      // early stop if close enough
		if (t > 100.) break;      // early stop if too far
	}

	// Coloring
	col = palette(t * .05 + float(i)*0.005);           // color based on distance(t) and on edge proximity (i)

	finalColor = vec4(col, 1);
}
