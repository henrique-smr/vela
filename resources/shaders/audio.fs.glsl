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
uniform float u_time;

void main(){
	vec2 st = 2.0*fragTexCoord.xy - 1;
	st.x *= u_resolution.x/u_resolution.y; // Aspect ratio correction

	float d = length(st);
	d = sin(d*10)/0.8;
	d = abs(d);
	d = step(0.1,d);
	finalColor = vec4(d,0.0,0.0,1.0);

}

void main_2()
{
	// Sample stereo audio data
	vec2 st = fragTexCoord;
	float rightChannel = texture(u_audio_channel_0, st).x;
	float leftChannel = texture(u_audio_channel_1, st).x;

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
	color *= (sin(u_time * 2.0) * 0.1 + 0.9);

	finalColor = vec4(color, 1.0);
}
