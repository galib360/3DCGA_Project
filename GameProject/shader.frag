#version 430

// Global variables for lighting calculations
layout(location = 1) uniform vec3 viewPos;
layout(location = 2) uniform sampler2D texShadow;
layout(location = 3) uniform float time;

layout(location = 4) uniform mat4 lightMVP;
layout(location = 5) uniform vec3 lightPos;
layout(location = 6) uniform sampler2D texLight;

// Output for on-screen color
layout(location = 0) out vec4 outColor;



// Interpolated output data from vertex shader
in vec3 fragPos;    // World-space position
in vec3 fragNormal; // World-space normal

vec4 light = vec4(0,3,0,0);

const vec3 lightColor = vec3(0.5, 0.5, 0.5);
const float lightPower = 5.0;
const vec3 ambColor = vec3(0.1, 0.1, 0.1);
const vec3 diffuseColor = vec3(0.4, 0.4, 0.4);
const vec3 specColor = vec3(1.0, 1.0, 1.0);
const float shininess = 16.0;

float random(vec3 seed, int i){
	vec4 seed4 = vec4(seed,i);
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
}



void main() {

	//vec3 lightPosxyz = vec3(lightPos.xyz);
	vec3 lightDir = normalize(light.xyz - fragPos);
	float visibility = 1.0;
	float bias = 0.002;

	vec3 normal = normalize(fragNormal);
	//vec3 lightDir = lightPos - fragPos;
	float distance = length(lightDir);
	distance = distance*distance;
	lightDir = normalize(lightDir);

	float lambertian = max(dot(lightDir, normal), 0.0);
	float specular = 0.0;

	if(lambertian> 0.0){
		vec3 viewDir = normalize(-fragPos);

		vec3 halfDir = normalize(lightDir + viewDir);
		float specAngle = max(dot(halfDir, normal), 0.0);
		specular = pow(specAngle, shininess);
	}

	vec2 poissonDisk[16] = vec2[]( 
	   vec2( -0.94201624, -0.39906216 ), 
	   vec2( 0.94558609, -0.76890725 ), 
	   vec2( -0.094184101, -0.92938870 ), 
	   vec2( 0.34495938, 0.29387760 ), 
	   vec2( -0.91588581, 0.45771432 ), 
	   vec2( -0.81544232, -0.87912464 ), 
	   vec2( -0.38277543, 0.27676845 ), 
	   vec2( 0.97484398, 0.75648379 ), 
	   vec2( 0.44323325, -0.97511554 ), 
	   vec2( 0.53742981, -0.47373420 ), 
	   vec2( -0.26496911, -0.41893023 ), 
	   vec2( 0.79197514, 0.19090188 ), 
	   vec2( -0.24188840, 0.99706507 ), 
	   vec2( -0.81409955, 0.91437590 ), 
	   vec2( 0.19984126, 0.78641367 ), 
	   vec2( 0.14383161, -0.14100790 ) 
	);


	vec4 shadowCoord = lightMVP * vec4(fragPos, 1.0);
	vec4 fragLightCoord = lightMVP * vec4(fragPos, 1.0);
	fragLightCoord.xyz /= fragLightCoord.w;
	//fragLightCoord.xyz = fragLightCoord.xyz * 0.5 + 0.5;
	float fragLightDepth = fragLightCoord.z;
	vec2 shadowMapCoord = fragLightCoord.xy;
	float shadowMapDepth = texture(texShadow, shadowMapCoord).x;


	//poisson sampling with PCF
	for (int i=0;i<4;i++){
		if ( texture( texShadow, shadowMapCoord + poissonDisk[i]/800.0 ).x  <  fragLightDepth-bias ){
				visibility-=0.2;
		}
	}


	//for spotlight
	//vec2 distancevec = fragLightCoord.xy - vec2(0.5, 0.5);
	//float distance = length(distancevec);
	//float lightMul = (-2* distance) + 1;
	

	
	vec3 finalColor = ambColor + (diffuseColor * lambertian * lightColor * lightPower / distance) + (specColor * specular * lightColor * lightPower / distance);
	outColor = vec4(finalColor, 1.0) * visibility; //with shadow only, no spotlight
	


}