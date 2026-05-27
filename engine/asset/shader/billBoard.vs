#version 330

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec4 vColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec3 fNormal;

void main() {
	fNormal = vNormal;
	
	mat4 viewModel = view * model;
	// expected direction in view space
	vec3 camDir_view = normalize(vec3(viewModel[3]));
	vec3 camRight_view = cross(vec3(0,1,0), camDir_view);
	vec3 camUp_view = cross(camDir_view, camRight_view);
	
	mat4 billBoardViewModel;
	billBoardViewModel[0] = vec4(camRight_view, 0);
	billBoardViewModel[1] = vec4(camUp_view, 0);
	billBoardViewModel[2] = vec4(camDir_view, 0);
	billBoardViewModel[3] = viewModel[3];
	
	gl_Position = proj * billBoardViewModel * vec4(vPos, 1.0);
}