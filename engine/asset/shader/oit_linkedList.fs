#version 430 core 

layout (early_fragment_tests) in;

layout (binding = 0 , r32ui) uniform uimage2D head_pointer_image;
layout (binding = 1, rgba32ui) uniform writeonly uimageBuffer list_buffer;
layout (binding = 0, offset = 0) uniform atomic_uint list_counter;

layout (location = 0) out vec4 outColor;

uniform float alpha_factor;

in VS_OUT{
    vec3 fragWorldPos;
    vec3 fragWorldNormal;
    vec2 fragUV;
} fs_in;

void main()
{
       //This is next pointer of linked list
       uint index;
	   uint old_head;
	   uvec4 item;
	   vec4 frag_color;
	   vec4 modulator;

	   index = atomicCounterIncrement(list_counter);
	   
	   old_head = imageAtomicExchange(head_pointer_image, ivec2(gl_FragCoord.xy), uint(index));

       vec3 normal = normalize(fs_in.fragWorldNormal);                                                         
       frag_color = vec4(1.0); // BlinnPhong() CalcLightInternal(light_color, light_direction, normal);  
	   modulator = vec4(frag_color.rgb,alpha_factor);
	  	      
	   item.x = old_head;
	   item.y = packUnorm4x8(modulator);
	   item.z = floatBitsToUint(gl_FragCoord.z);
	   item.w = 0;
	   
	   imageStore(list_buffer,int(index), item);

	   outColor = frag_color;
}
