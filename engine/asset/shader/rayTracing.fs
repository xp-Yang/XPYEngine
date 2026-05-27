#version 330 core
out vec4 FragColor;

in vec2 fragUV;

struct Camera {
    vec3 pos;
    float distance; // 到此处假定的平面的距离，实际上是任意值都可以。射线方向由fov和aspect_ratio决定
    float fov;
    float aspect_ratio;
    vec3 front;
    vec3 right;
    vec3 up;
};
uniform struct Camera camera;

struct Ray{
    vec3 origin;
    vec3 direction;
    float t;
};

struct Sphere{
    vec3 origin;
    float radius;

    //material
    vec3 albedo;
    float fuzzy;

    bool is_metal;
};

uniform float randOrigin;

uint wseed;
float randcore(uint seed) {
    seed = (seed ^ uint(61)) ^ (seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> uint(4));
    seed *= uint(0x27d4eb2d);
    wseed = seed ^ (seed >> uint(15));
    return float(wseed) * (1.0 / 4294967296.0);
}
float rand() {
    return randcore(wseed);
}
vec3 randomUnitVec() {
    while (true) {
        vec3 unit_cube = 2.0 * vec3(rand(), rand(), rand()) - vec3(1, 1, 1);
        if (dot(unit_cube, unit_cube) < 1.0f) { //为了均匀分布，否则归一化后沿立方体对角线的抽样比较多
            return normalize(unit_cube);
        }
    }
    return vec3(1.0f, 0, 0);
}

vec3 getPointOnUnitSphere(vec3 plane_point, vec3 normal) {
    float r_square = dot(plane_point, plane_point);
    float h = sqrt(1 - r_square);
    return plane_point + h * normal;
}
 // 单位圆内的点按极轴均匀分布(非均匀分布)
vec2 randomInUnitCircleByPolar() {
    float r = rand();
    float theta = 2 * 3.14159 * rand();
    return vec2(r * cos(theta), r * sin(theta));
}
vec3 randomLambertianDistribution(vec3 normal) {
    // 1. 构造切平面
    vec3 up = vec3(0, 1, 0);
    vec3 local_u;
    if (normal == up)
        local_u = vec3(1, 0, 0);
    else
        local_u = cross(normal, up);
    vec3 local_v = cross(normal, local_u);

    // 2. 在切平面的单位圆内均匀取点
    vec3 random_plane_point;
    vec2 coef = randomInUnitCircleByPolar();
    random_plane_point = coef.x * local_u + coef.y * local_v;

    // 3. 将点映射回球面
    return getPointOnUnitSphere(random_plane_point, normal);
}

vec3 randomOnHemisphere(vec3 normal) {
    vec3 unit = randomUnitVec();
    if (dot(unit, normal) > 0.0)
        return unit;
    else
        return -unit;
}

float valid_range;
float hitSphere(Ray ray, Sphere sphere){
    // solve the intersection equation
    vec3 rayOrigin = ray.origin;
    vec3 rayDir = ray.direction;
    vec3 sOrigin = sphere.origin;
    float sRadius = sphere.radius;

    float a = dot(rayDir, rayDir);
    float b = 2 * dot((rayOrigin - sOrigin), rayDir);
    float c = dot((rayOrigin - sOrigin), (rayOrigin - sOrigin)) - sRadius * sRadius;
    float discriminant = b * b - 4 * a * c;
    if(discriminant >= 0){
        float root = (-b - sqrt(discriminant)) / (2.0 * a);
        if(root <= 0 || root >= valid_range)
            return -1.0f;
        else 
            return root;
    }
    else{
        return -1.0f;
    }
}

struct HitResult{
    bool hit;
    vec3 point;
    vec3 normal;

    vec3 albedo;
    float fuzzy;

    bool is_metal;
};
// 一次碰撞（无反射）
HitResult hitOnce(Ray ray, Sphere[3] sphereList){
    //初始化最近的解
    valid_range = 999999;
    //求最近的点
    HitResult result;
    result.hit = false;
    float root;
    int closest_id;
    for(int i = 0; i < 3; i++){
        //求根： 线-球
        root = hitSphere(ray, sphereList[i]);
        if(root > 0.0f){
            valid_range = root;
            result.hit = true;
            closest_id = i;
        }
    }
    if(result.hit){
        vec3 intersectionPoint = ray.origin + valid_range * ray.direction;
        vec3 N = normalize(intersectionPoint - sphereList[closest_id].origin);

        result.point = intersectionPoint;
        result.normal = N;
		result.albedo = sphereList[closest_id].albedo;
		result.fuzzy = sphereList[closest_id].fuzzy;
		result.is_metal = sphereList[closest_id].is_metal;
        //递归查看能不能再hit。
        //GLSL不能递归，在上一层循环
    }
    return result;
}

vec3 shading(Ray ray, Sphere[3] sphereList) {
	vec3 color = vec3(1.0, 1.0, 1.0);
	bool hitAnything = false;
    int reflect_depth = 20;
	for (int i = 0; i < reflect_depth; i++) {
        HitResult hitRes = hitOnce(ray, sphereList);
		if (hitRes.hit) {
            vec3 incident_dir = ray.direction;
			ray.origin = hitRes.point;
            if(!hitRes.is_metal){
			    ray.direction = normalize(hitRes.normal + randomUnitVec());
			    //ray.direction = normalize(randomOnHemisphere(hitRes.normal));
			    //ray.direction = normalize(randomLambertianDistribution(hitRes.normal));
            }
            else{
			    ray.direction = normalize(hitRes.normal + reflect(incident_dir, hitRes.normal) + hitRes.fuzzy * randomUnitVec());
            }
			color *= hitRes.albedo;
			hitAnything = true;
		}
		else {
			break;
		}
	}
	if(!hitAnything) color = vec3(0.0, 0.0, 0.0);
	return color;
}

void main() {
    //初始化随机种子
    wseed = uint(randOrigin * (fragUV.x * fragUV.y));
    //1. 构造ray
	Ray ray;
	ray.origin = camera.pos;

    float width = tan(camera.fov / 2) * camera.distance * 2.0;
    float height = width / camera.aspect_ratio;
    vec3 leftbottom = camera.pos + camera.distance * camera.front - width / 2.0 * camera.right - height / 2.0 * camera.up;
	ray.direction = normalize(leftbottom + (fragUV.x * width) * camera.right + (fragUV.y * height) * camera.up - camera.pos);
    //2. 构造sphere
    Sphere[3] sphereList;
        Sphere sphere0;
    sphere0.origin = vec3(-5.0, 4.0, -1.0);
    sphere0.radius = 4.0f;
    sphere0.albedo = vec3(0.8f, 0.8f, 0.1f);
    sphere0.fuzzy = 0.2f;
    sphere0.is_metal = false;
    sphereList[0] = sphere0;
        Sphere sphere1;
    sphere1.origin = vec3(5.0, 5.0, -1.0);
    sphere1.radius = 5.0f;
    sphere1.albedo = vec3(0.5f, 0.5f, 0.9f);
    sphere1.fuzzy = 0.4f;
    sphere1.is_metal = false;
    sphereList[1] = sphere1;
        Sphere sphere2;
    sphere2.origin = vec3(0.0, -1000.0, -1.0);
    sphere2.radius = 1000.0f;
    sphere2.albedo = vec3(0.5f, 0.5f, 0.5f);
    sphere2.fuzzy = 0.0f;
    sphere2.is_metal = false;
    sphereList[2] = sphere2;
    //3. shading color by hit, Monte Carlo it
    vec3 color = vec3(0.0f, 0.0f, 0.0f);
    int frames_samples = 1;
    for(int i = 0; i < frames_samples; i++){
        color += shading(ray, sphereList) / frames_samples;
    }
    FragColor = vec4(color, 1.0f);

    //debug

    ////vec2 random = randomInUnitCircleByPolar();
    //
    //vec3 test = normalize(vec3(0, 1, 0) + randomUnitVec());
    //vec2 random = vec2(test.x, test.z);
    //
    //random = (1 + random) / 2.0;
    //
    //if(abs(fragUV.x - random.x) < 0.05 && abs(fragUV.y - random.y) < 0.05){
    //  FragColor = vec4(1.0f);
    //}else
    //  FragColor = vec4(0.0f);
}