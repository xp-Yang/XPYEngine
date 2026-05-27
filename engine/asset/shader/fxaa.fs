#version 330

in vec2 fragUV;
out vec4 outColor;

uniform sampler2D mainTexture;


float calcLuminance(vec4 color) {
	return dot(vec3(color), vec3(0.213, 0.715, 0.072));
}

float calcContrast(float N_luma, float W_luma, float S_luma, float E_luma, float M_luma) {
	float max_luminance = max(N_luma,
	 max(W_luma,
	  max(S_luma,
	   max(E_luma, M_luma))));
	float min_luminance = min(N_luma,
	 min(W_luma,
	  min(S_luma,
	   min(E_luma, M_luma)))); 
	float contrast = max_luminance - min_luminance;
	return contrast;
}

vec2 calcEdgeNormal(float N_luma, float W_luma, float S_luma, float E_luma, float M_luma,
	float NW_luma, float NE_luma, float SW_luma, float SE_luma)
{
	float gradHorizontal = 2 * ((N_luma - M_luma) - (M_luma - S_luma)) + (NW_luma - W_luma) - (W_luma - SW_luma) + (NE_luma - E_luma) - (E_luma - SE_luma);
	float gradVertical = 2 * ((E_luma - M_luma) - (M_luma - W_luma)) + (NE_luma - N_luma) - (N_luma - NW_luma) + (SE_luma - S_luma) - (S_luma - SW_luma);
	bool isHorizontal = abs(gradHorizontal) > abs(gradVertical);
	
	// myIsEdgeGradHorizontal
	//gradHorizontal = (N_luma - M_luma) + (M_luma - S_luma);
	//gradVertical = (E_luma - M_luma) + (M_luma - W_luma);
	//isHorizontal = abs(gradHorizontal) > abs(gradVertical);
	
	vec2 normal = vec2(0.0);
	if (isHorizontal)
		normal.y = sign(abs(N_luma - M_luma) - abs(M_luma - S_luma));
	else
		normal.x = sign(abs(E_luma - M_luma) - abs(M_luma - W_luma));
	return normal;
}

#define EDGE_THRESHOLD_MIN 0.0312
#define EDGE_THRESHOLD_MAX 0.125

void main() {
	vec2 tex_offset = 1.0 / textureSize(mainTexture, 0);
	vec2 st = fragUV;
	vec4 N = texture(mainTexture, vec2(st.x, st.y + tex_offset.y));
	vec4 S = texture(mainTexture, vec2(st.x, st.y - tex_offset.y));
	vec4 W = texture(mainTexture, vec2(st.x - tex_offset.x, st.y));
	vec4 E = texture(mainTexture, vec2(st.x + tex_offset.x, st.y));
	vec4 NW = texture(mainTexture, vec2(st.x - tex_offset.x, st.y + tex_offset.y));
	vec4 NE = texture(mainTexture, vec2(st.x + tex_offset.x, st.y + tex_offset.y));
	vec4 SW = texture(mainTexture, vec2(st.x - tex_offset.x, st.y - tex_offset.y));
	vec4 SE = texture(mainTexture, vec2(st.x + tex_offset.x, st.y - tex_offset.y));
	vec4 M = texture(mainTexture, st);
	float N_luma = calcLuminance(N);
	float W_luma = calcLuminance(W);
	float S_luma = calcLuminance(S);
	float E_luma = calcLuminance(E);
	float NW_luma = calcLuminance(NW);
	float NE_luma = calcLuminance(NE);
	float SW_luma = calcLuminance(SW);
	float SE_luma = calcLuminance(SE);
	float M_luma = calcLuminance(M);
	float max_luminance = max(N_luma,
	 max(W_luma,
	  max(S_luma,
	   max(E_luma, M_luma))));

	float contrast = calcContrast(N_luma, W_luma, S_luma, E_luma, M_luma);
	if (contrast < max(EDGE_THRESHOLD_MIN, max_luminance * EDGE_THRESHOLD_MAX)) {
		discard;
	}

	vec2 normal = calcEdgeNormal(N_luma, W_luma, S_luma, E_luma, M_luma, NW_luma, NE_luma, SW_luma, SE_luma);
	vec4 color = vec4(0.0);	
	float expect_luma = (N_luma + W_luma + S_luma + M_luma) * 0.25;
	float luma_delta = abs(M_luma - expect_luma);
	float blend = luma_delta / contrast;
	color = texture(mainTexture, st + normal * blend * tex_offset);
	outColor = color;
	//outColor = ((N + W + S + E) + (NW + NE + SW + SE)) / 8;
	//outColor = M;
	//outColor = vec4(vec3(contrast), 1.0);
}