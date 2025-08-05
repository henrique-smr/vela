#version 330
// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Output fragment color
out vec4 finalColor;

uniform vec2 u_resolution;
uniform float u_signal;
uniform float u_time;

float random (in vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))
                * (43758.5453123));
}

// Value noise by Inigo Quilez - iq/2013
// https://www.shadertoy.com/view/lsf3WH
float noise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);
    vec2 u = f*f*(3.0-2.0*f);
    return mix( mix( random( i + vec2(0.0,0.0) ),
                     random( i + vec2(1.0,0.0) ), u.x),
                mix( random( i + vec2(0.0,1.0) ),
                     random( i + vec2(1.0,1.0) ), u.x), u.y);
}

mat2 rotate2d(float angle){
    return mat2(cos(angle),-sin(angle),
                sin(angle),cos(angle));
}

float lines(in vec2 pos, float b){
    float scale = 10.0;
    pos *= scale;
    return smoothstep(0.0,
                    .5+b*.5,
                    abs((sin(pos.x*3.1415)+b*2.0))*.5);
}

void main() {
    vec2 st = fragTexCoord;

    vec2 pos = st.yx*vec2(10.,3.);

    float pattern = pos.x;


    // Add noise
    pos =  noise(pos+vec2(u_time*0.1)) * pos;

    // Draw lines
    pattern = lines(pos,.5);

	float signal = u_signal*0.0000001;
	float time = u_time*0.01;

	vec3 color = vec3(noise(pos + vec2(time, signal)));

	 // Add some color variation
	 color *= vec3(0.5 + 0.5 * sin(time + pos.x * 0.1), 
						0.5 + 0.5 * cos(time + pos.y * 0.1), 
						0.5 + 0.5 * sin(time + pos.x * pos.y * 0.1));

	 // Apply pattern to color
	 color *= pattern;

	 // Final color
	finalColor = vec4(color.x, color.x*color.y, color.y, 1.0);

    // finalColor = vec4(vec3(pattern,,1.0);
}
