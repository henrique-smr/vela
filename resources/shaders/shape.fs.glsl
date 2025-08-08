#version 330
// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Output fragment color
out vec4 finalColor;

uniform float u_signal;
uniform float u_time;
uniform vec2 u_resolution;

uniform sampler2D u_audio_channel_0;
uniform sampler2D u_audio_channel_1;
uniform sampler2D u_spectrum_channel_0;
uniform sampler2D u_spectrum_channel_1;

vec2 random2(vec2 st){
    st = vec2( dot(st,vec2(127.1,311.7)),
              dot(st,vec2(269.5,183.3)) );
    return -1.0 + 2.0*fract(sin(st)*43758.5453123);
}

// Gradient Noise by Inigo Quilez - iq/2013
// https://www.shadertoy.com/view/XdXGW8
float noise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    vec2 u = f*f*(3.0-2.0*f);

    return mix( mix( dot( random2(i + vec2(0.0,0.0) ), f - vec2(0.0,0.0) ),
                     dot( random2(i + vec2(1.0,0.0) ), f - vec2(1.0,0.0) ), u.x),
                mix( dot( random2(i + vec2(0.0,1.0) ), f - vec2(0.0,1.0) ),
                     dot( random2(i + vec2(1.0,1.0) ), f - vec2(1.0,1.0) ), u.x), u.y);
}

mat2 rotate2d(float _angle){
    return mat2(cos(_angle),-sin(_angle),
                sin(_angle),cos(_angle));
}

float shape(vec2 st, float radius) {

	st = vec2(0.85,0.5)-st;
	float r = length(st)*2.0;
	float a = atan(st.y,st.x);
	// float m = abs(mod(a+u_time*2.,3.14*2.)-3.14)/3.6;
	float m = abs(mod(a,3.14))/(3.14);
	float f = (radius );

	f += texture(u_audio_channel_0, vec2(m,0.1)).x*0.1;
	// float leftChannel = texture(u_spectrum_channel_1, fragTexCoord).x;

	// m += noise(st+u_time*0.1)*.5;
	// a *= 1.+abs(atan(u_time*0.2))*.1;
	// a *= 1.+noise(st+u_time*0.1)*0.1;
	// f += sin(a*50.)*noise(st+u_time*.2)*.1;
	// f += (sin(a*21.)*.1*pow(m,2.));
	return 1.-smoothstep(f,f+0.007,r);
}

float shapeBorder(vec2 st, float radius, float width) {
    return shape(st,radius)-shape(st,radius-width);
}

void main() {
	vec2 st = fragTexCoord;
	st.x *= u_resolution.x / u_resolution.y; // Adjust for aspect ratio
	vec3 color = vec3(1.0) * shapeBorder(st,0.8,0.02);

	finalColor = vec4( 1.-color, 1.0 );
}
void main_bak()
{
	// Sample stereo audio data
	vec2 st = fragTexCoord;
	st.x *= u_resolution.x / u_resolution.y; // Adjust for aspect ratio
	float rightChannel = texture(u_spectrum_channel_0, st).x;
	float leftChannel = texture(u_spectrum_channel_1, st).x;

	// Calculate amplitude and phase information
	float amplitude = length(vec2(leftChannel, rightChannel));
	float stereoWidth = abs(leftChannel - rightChannel);

	// Create color based on audio characteristics
	vec3 color;

	// Color mapping based on stereo characteristics
	if (amplitude > 0.001)
	{
		// Hue based on left/right balance
		float balance = (leftChannel + 1.0) / 2.0; // Normalize to 0-1

		// Create color gradient
		color.x = amplitude * (1.0 - balance) * 2.0;           // More red for left
		color.y = amplitude * min(balance * 2.0, (1.0 - balance) * 2.0); // Green for center
		color.z = amplitude * balance * 2.0;                    // More blue for right

		// Add stereo width information
		color += vec3(stereoWidth * 0.5);
	}
	else {
		color = vec3(0.0); // Default color if no audio
	}

	// Add some animation
	// color *= (sin(u_time * 2.0) * 0.1 + 0.9);

	finalColor = vec4(color, 1.0);
}
// vec3 getRayDir(
// vec3 camDir,
// vec3 camUp,
// vec2 texCoord) {
// vec3 camSide = normalize(
// cross(camDir, camUp));
// vec2 p = 2.0 * texCoord - 1.0;
// p.x *= iResolution.x
// / iResolution.y;
// return normalize(
// p.x * camSide
// + p.y * camUp
// + iPlaneDist * camDir);
// }

