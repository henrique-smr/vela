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

#define audio0(_x) (texture(u_audio_channel_0, vec2(_x, 0.0)).x)
#define audio1(_x) (texture(u_audio_channel_1, vec2(_x, 0.0)).x)
#define spectrum0(_x) (texture(u_spectrum_channel_0, vec2(_x, 0.0)).x)
#define spectrum1(_x) (texture(u_spectrum_channel_1, vec2(_x, 0.0)).x)

float avgFreq(float start, float end, float step) {
    float div = 0.0;
    float total = 0.0;
    for (float pos = start; pos < end; pos += step) {
        div += 2.0;
        total += texture(u_spectrum_channel_0, vec2(pos)).x;
        total += texture(u_spectrum_channel_1, vec2(pos)).x;
    }
    return total / div;
}
float random1D (in float _st) {
	 return fract(sin(_st * 12.9898 + 78.233) * 43758.5453123);
}
vec2 random2D (in vec2 _st) {
    return fract(
		sin(
			(_st.xy * vec2(12.9898,78.233))
		) * 43758.5453123
	);
}
vec3 random3D (in vec3 _st) {
	 return fract(
		sin(
			(_st.xyz * vec3(12.9898, 78.233, 45.164))
		) * 43758.5453123
	);
}

float noise1D (in float x) {
	float i = floor(x);  // integer
	float f = fract(x);  // fraction
	float a = smoothstep(0.,1.,f);
	return mix(random1D(i), random1D(i + 1.0), a);
}
float noise2D(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    vec2 u = f*f*(3.0-2.0*f);

    return mix( mix( dot( random2D(i + vec2(0.0,0.0) ), f - vec2(0.0,0.0) ),
                     dot( random2D(i + vec2(1.0,0.0) ), f - vec2(1.0,0.0) ), u.x),
                mix( dot( random2D(i + vec2(0.0,1.0) ), f - vec2(0.0,1.0) ),
                     dot( random2D(i + vec2(1.0,1.0) ), f - vec2(1.0,1.0) ), u.x), u.y);
}
vec3 palette( in float t ) {
	float bassFreq = pow(avgFreq(0.0, 0.1, 0.01), 0.85);
	float medFreq = pow(avgFreq(0.10, 0.6, 0.01), 0.85);
	float topFreq = pow(avgFreq(0.6, 1.0, 0.01), 0.85);
	// vec3 a = vec3(0.5, 0.5, 0.5);
	// vec3 b = vec3(0.5, 0.5, 0.5);
	// vec3 c = vec3(2.0, 1.0, 0.);
	// vec3 d = vec3(0.50, 0.20, 0.25);
	vec3 a = vec3(0.5, 0.5, 0.5);
	vec3 b = vec3(0.5, 0.5, 0.5);
	vec3 c = vec3(1.0, 1.0, 1.) + vec3(
		// 0
		bassFreq*2. ,
		bassFreq *2.,
		bassFreq *2.
		// noise2D(vec2(u_time*0.5,bassFreq*2.)),
		// noise2D(vec2(u_time*0.5,bassFreq *2.)),
		// noise2D(vec2(u_time*0.5,bassFreq *2.))
	);
	vec3 d = vec3(0.00, 0.10, 0.20) + vec3(
		0,
		0,// bassFreq *.5,
		0
		// noise2D(vec2(u_time*0.5,topFreq*2.)),
		// noise2D(vec2(u_time*0.5,topFreq *2.)),
		// noise2D(vec2(u_time*0.5,topFreq *2.))
	);
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
float sdTorus( vec3 p, vec2 t ) {
  vec2 q = vec2(length(p.xz)-t.x,p.y);
  return length(q)-t.y;
}
float sdOctahedron( vec3 p, float s) {
  p = abs(p);
  return (p.x+p.y+p.z-s)*0.57735027;
}

// Scene distance
float map(vec3 p) {
	vec3 q = p;

	q.z += u_time*1.;
	// q.x += noise1D(u_time); // random value for each ray
	// q.y += noise2D(vec2(u_time,0.323618127693876123)); // random value for each ray
	// q.x += fract(u_time*0.5);
	// q.y += fract(u_time*0.25);

	//Space repeatition
	q.xy = mod(q.xy, 1.) - .5;
	q.z = mod(q.z, .5) - .25;

	float f = fract(q.z);
	vec3 c = vec3(0.,0.,0.);
	float r =  audio0(f)*0.1;
	// float r = 0.1; // radius of the octahedron

	// float sphereRadi = noise2D(vec2(u_time*0.1, 0.12342))*0.6 + 0.3; // Random radius for the sphere
	float octaRadi = noise2D(vec2(u_time*0.1, .3432))*0.6 + 0.2; // Random radius for the octahedron

	// float sphere = sdSphere(q - c, sphereRadi + r);

	q.yz = rot2D(u_time*.1) * q.yz; // Rotate the scene based on time


	float oct = sdOctahedron(q, octaRadi + r); // Octahedron distance
	return oct; // distance to a sphere of radius 1

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
	rd.xy *= rot2D(noise2D(vec2(u_time*0.1,0.12342))*3.);
	rd.yz *= rot2D(noise2D(vec2(u_time*0.1,.3432))*3.);
	// ro.xy *= rot2D(noise2D(vec2(u_time*0.1,2.12342))*2.);
	// ro.yz *= rot2D(noise2D(vec2(u_time*0.1,3.3432))*2.);

	// Raymarching
	int i;
	for (i = 0; i < 80; i++) {
		vec3 p = ro + rd * t;     // position along the ray

		float rr = noise1D(u_time*0.1)*2 - 1; // random value for each ray

		p.xy *= rot2D(t * 0.35*rr); // Rotate the rays 
		// p.y += sin(t*2.) * 0.15;

		float d = map(p);         // current distance to the scene

		t += d;                   // "march" the ray

		if (d < .001) break;      // early stop if close enough
		if (t > 100.) break;      // early stop if too far
	}

	// Coloring
	col = palette(t * .05 + float(i)*0.005);           // color based on distance(t) and on edge proximity (i)

	finalColor = vec4(col, 1);
}
