#version 440

uniform sampler2D TexSampler2D;
uniform sampler3D TexSampler3D;
uniform samplerCube TexSamplerCube;
uniform int TexCase, Divide;
uniform bool UseEnvMap, LocalViewer, UseLine;
uniform mat3 CubeMapRotInvMat;

in vec2 TexCoord2D;
in vec3 TexCoord3D, ecPosition, tnorm, bary, oriBary;
in flat float f_isStrange;
out vec4 FragColor;

struct PointLightSource
{
	vec4 position, ambient, diffuse, specular;
	float constantAttenuation, linearAttenuation, quadraticAttenuation;
};
uniform PointLightSource PointLightSource0;

struct Material
{
	vec4 ka, kd, ks;
};
uniform Material Mtl;

void PointLight(const vec3 eye, const vec3 ecPosition3, const vec3 normal,
				inout vec4 ambient, inout vec4 diffuse, inout vec4 specular)
{
	vec3 VP = vec3(PointLightSource0.position) - ecPosition3;	// 片元到光源的向量
	float d = length(VP);										// 片元和光源的距离
	VP = normalize(VP);
	float attenuation = 1.0 / (PointLightSource0.constantAttenuation +
						 PointLightSource0.linearAttenuation * d +
						 PointLightSource0.quadraticAttenuation * d * d);// 衰减因子
// 方法1:
	float nDotHV = max(0.0, dot(reflect(-VP, normal), eye));
// 方法2:
	//halfVector = normalize(VP + eye);		// direction of maximum highlights
	//nDotHV = max(0.0, dot(normal, halfVector));

	float nDotVP = max(0.0, dot(normal, VP));	// normal.light
	float pf = 0.0;								// power factor
	if (nDotVP > 0.0)
		pf = pow(nDotHV, 10.0);

	ambient += PointLightSource0.ambient * attenuation;
	diffuse += PointLightSource0.diffuse * nDotVP * attenuation;
	specular += PointLightSource0.specular * pf * attenuation;
}

void main()
{

//	FragColor.r = f_isStrange;
//	FragColor.g = 1 - f_isStrange;
//	FragColor.b = 0;
//	return;

	vec3 eye = vec3(0.0, 0.0, 1.0);
	if (LocalViewer)
		eye = -normalize(ecPosition);
	vec4 amb = vec4(0.0), diff = vec4(0.0), spec = vec4(0.0);
	PointLight(eye, ecPosition, normalize(tnorm), amb, diff, spec);
	FragColor = amb * Mtl.ka + diff * Mtl.kd;
	vec4 SecondaryColor = spec * Mtl.ks;
	FragColor = clamp(FragColor, 0.0, 1.0);

	if (TexCase == 2)
	{
		//FragColor = texture(TexSampler2D, TexCoord2D);
		FragColor *= texture(TexSampler2D, TexCoord2D);
	}
	else if (TexCase == 3)
	{
		FragColor *= texture(TexSampler3D, TexCoord3D);
	}
	if (UseEnvMap)
	{
		vec3 ReflectDir = CubeMapRotInvMat * reflect(ecPosition, tnorm);
		ReflectDir.z = -ReflectDir.z;
		FragColor = texture(TexSamplerCube, ReflectDir);
	}
	FragColor += SecondaryColor;
	FragColor = clamp(FragColor, 0.0, 1.0);

	//FragColor = vec4(0.0, 0.0, 0.0, 1.0);

	/******************** 开始绘制线框图 *********************/
	if (UseLine)
	{
		float limit = 0.004;
		//float limit = 0.012;
		// 线框图中最小的三角形，蓝色
		for (int i = 0; i <= Divide; ++i)
		{
			if (abs(bary.x - 1.0 * i / Divide) < limit ||
				abs(bary.y - 1.0 * i / Divide) < limit ||
				abs(bary.z - 1.0 * i / Divide) < limit)
				FragColor = vec4(1.0, 1.0, 1.0, 1.0);
		}

		// 线框图中切割后的三角形，中等大小，黄色
		if (bary.x < limit || bary.y < limit || bary.z < limit)
			FragColor = vec4(1.0, 1.0, 0.0, 1.0);
			//FragColor = vec4(1.0, 0.0, 0.0, 1.0);
		if (bary.x < limit *2|| bary.y < limit*2 || bary.z < limit*2)
			//FragColor = vec4(0.0, 1.0, 0.0, 1.0);

		// 线框图中切割前的三角形，最大，红色
		if (oriBary.x < limit || oriBary.y < limit || oriBary.z < limit)
			FragColor = vec4(1.0, 0.0, 0.0, 1.0);
	}

}
