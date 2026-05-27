vec3 BlinnPhong(vec3 light_color, vec3 n, vec3 v, vec3 l, vec3 diffuse_coef, vec3 specular_coef, float shininess)
{
    vec3 diffuse_light = light_color * max(dot(l, n), 0.0) * diffuse_coef;
    
    vec3 h = normalize(v + l);
    vec3 specular_light = light_color * pow(max(dot(n, h), 0.0), shininess) * specular_coef;

    // phong
    //vec3 reflect_dir = reflect(-l, n);
    //vec3 specular_light = light_color * pow(max(dot(v, reflect_dir), 0.0), 128.0) * specular_coef;

    return diffuse_light + specular_light;
}
#line 1
