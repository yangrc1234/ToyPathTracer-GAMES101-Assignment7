#include "Material.hpp"

Vector3f toWorld(const Vector3f& a, const Vector3f& N);

/*Clamp a value to range [0,1]*/
float saturate(float t) {
	return std::clamp(t, 0.0f, 1.0f);
}

/*Power5 of a value*/
float pow5(float v) {
	return v * v * v * v * v;
}

/*GGX Geometry shadow term. */
float Visibility(float vn, float vh, float roughness) {
	if (vh * vn <= 0.0f)
		return 0.0f;
	float vh2 = vh * vh;
	float tan2 = (1.0f - vh2) / vh2;
	return 2.0f / (1 + std::sqrt(1.0f + roughness * roughness * tan2));
}

/*GGX D term. Indicate given N, H, roughness, how much */
float GGXTerm(float ndoth, float roughness) {
	if (ndoth <= 0.0f)
		return 0.0f;
	float a2 = roughness * roughness;
	float d = (ndoth * a2 - ndoth) * ndoth + 1.0f;
	return a2 / (M_PI * d * d + EPSILON);
}

/*Given normal and microsurface normal(half direction), calculate probablity of the microsurface.*/
float GGXHalfPDF(Vector3f n, Vector3f h, float roughness) {
	return GGXTerm(dotProduct(n, h), roughness) * abs(dotProduct(n, h));
}

/*Convert smoothness to roughness*/
float SmoothnessToRoughenss(float smoothness) {
	return std::max(0.002f, (1.0f - smoothness) * (1.0f - smoothness));
}

/*
Given normal and roughness, generate a half dir sample.
*/
Vector3f SampleGGXSpecularH(Vector3f N, float roughness) {
	float d1 = get_random_float(), d2 = get_random_float();
	float theta = std::atan2(roughness * std::sqrt(d1), std::sqrt(1.0f - d1));
	float phi = 2.0f * M_PI * d2;
	Vector3f microNLocal(
		std::sin(theta) * std::cos(phi),
		std::sin(theta) * std::sin(phi),
		std::cos(theta)
		);

	auto microNWorld = toWorld(microNLocal, N).normalized();
	
	return microNWorld;
}

/*Given N, w_i, w_o and material ior, get ior of the media w_i in, and ior of the media w_o in*/
void GetInsideOutsideIOR(Vector3f N, Vector3f wi, Vector3f wo, float matIor, float& ior_i, float& ior_o) {
	float nl = dotProduct(N, wi);
	float nv = dotProduct(N, wo);
	//Calculate half dir.
	if (nl < 0.0f) {
		ior_i = matIor;
	}
	else {
		ior_i = 1.0f;
	}
	if (nv < 0.0f) {
		ior_o = matIor;
	}
	else {
		ior_o = 1.0f;
	}
}

/*
Given normal, w_i, w_o and material ior, calculate half vector.
If w_i, w_o is on the same side, (w_i + w_o).normalized().
Otherwise refraction is considered, and the half direction is calculated according t othe GGX paper.(Which is exactly normal of the plane that refracts the w_i or w_o into one another)*/
Vector3f GetHalfDir(Vector3f N, Vector3f wi, Vector3f wo, float matIor) {

	float nl = dotProduct(N, wi);
	float nv = dotProduct(N, wo);
	if (nl == 0.0f || nv == 0.0f)
		return 0.0f;
	Vector3f h;
	if (nl * nv > 0.0f)      //Reflection.
	{
		h = (wi + wo).normalized();
		if (nv < 0.0f)
			h = -h;
	}
	else {
		//Calculate half dir.
		if (nv < 0.0f) {
			h = -(matIor * wo + wi).normalized();
		}
		else {
			h = -(wo + wi * matIor).normalized();
		}
	}
	return h;
}

/*
Given a vector a that is located in local system, transform the local system to world system such that z-axis in local system aligns with world system's one.
*/
Vector3f toWorld(const Vector3f& a, const Vector3f& N) {
	Vector3f tangent = anyPerpendicular(N), bitangent = crossProduct(N, tangent);

	return Vector3f(
		a.x * tangent.x + a.y * bitangent.x + a.z * N.x,
		a.x * tangent.y + a.y * bitangent.y + a.z * N.y,
		a.x * tangent.z + a.y * bitangent.z + a.z * N.z
		);
}

/*Draw a sample from diffuse distribution, which is cosine-weighted hemisphere. */
Vector3f getDiffuseSampleAndPdf(const Vector3f& wo, const Vector3f& N, float& pdf) {
	//cosine-weighted sampling.
	float u1 = get_random_float();
	float r = std::sqrt(u1);
	float theta = 2 * M_PI * get_random_float();
	float x = r * cos(theta), y = r * sin(theta);

	Vector3f wi = toWorld(Vector3f(x, y, std::sqrt(1.0f - u1)), N).normalized();
	pdf = dotProduct(wi, N) / (M_PI);
	return wi;
}

/*Given w_o, N, w_i, calculate its probablity under Lambert lighting model.*/
float getDiffusePdf(const Vector3f& wo, const Vector3f& N, const Vector3f& wi) {
	return saturate(dotProduct(wi, N)) / (M_PI); //cosine-weighted.
}

/*
Given w_i, w_o, N, calculate how much light would be passed per unit solid angle.
Note that return value is not BSDF, since we combine the cosine-term( dot(normal, light) ) calculation here.

The code is just an plain implementation of Walter07. See the paper for why and how.
*/
Vector3f Material::evalGivenSample(const Vector3f& wo, const Vector3f& wi, const Vector3f& N) {
	float nl = dotProduct(N, wi);
	float nv = dotProduct(N, wo);
	if (nl == 0.0f || nv == 0.0f)
		return 0.0f;
	Vector3f h = GetHalfDir(N, wi, wo, ior_d);

	float nh = dotProduct(N, h);
	float lv = dotProduct(wi, wo);
	float lh = dotProduct(wi, h);
	float vh = dotProduct(wo, h);
	float D = GGXTerm(nh, rough);
	float G = Visibility(nv, vh, rough) * Visibility(nl, lh, rough);
	Vector3f f = fresnel(wi, h);

	if (nl * nv > 0.0f)      //Reflection.
	{
		Vector3f specular = 0;
		if (G != 0.0f)
			specular = (D * f * G) / (4.0 * abs(nv));       //Original formula is DFG/(4*nv*nl), we combine the cosine term here, so nl canceld out with it.

		Vector3f diffuse = 0;
		if (this->m_type == Dieletric) {
			diffuse = this->Kd * (Vector3f::One() - f) / M_PI * saturate(nl);
		}

		return diffuse + specular;
	}
	else {      //Refraction
		//Calculate half dir.
		if (m_type != TransmittanceDieletric)
			return 0.0f;
		float ior_i, ior_o;
		GetInsideOutsideIOR(N, wi, wo, ior_d, ior_i, ior_o);
		if (nv < 0.0f) {
			ior_i = 1.0f; ior_o = ior_d;
		}
		else {
			ior_i = ior_d; ior_o = 1.0f;
		}
		auto partA = abs(vh) * abs(lh) / (abs(nv) /* abs(nl)*/);	//Cancelled out with cosine term.
		auto partB = ior_o * ior_o * (1.0f - f.x) * G * D;
		if (partA * partB == 0.0f)
			return 0.0f;
		auto partC = ior_i * lh + ior_o * vh;
		partC *= partC;

		return partA * partB / partC;
	}
}

/*
The GGX microfacet model doesn't include diffuse surface calculation.
It's possible to create a diffuse material based on Lambert.
But if we want smooth dieletric(plastic pipe etc.), diffuse is important as well as specular.

To implement such a combined bsdf model, just "combine" diffuse pdf and specular pdf.
You don't have to really get a pdf formula of it. Here's the simple way.

When draw sample:
1. flip a coin(equal chance) to decide use diffuse or specular.
2. Draw a sample from the chosen pdf.
3. Calculate the sample's probablity on the other model not chosed.
4. Sum the probablity up, divie by 2.0f as the returned probablity(if in 1. step the chance is not equal, here should be adjusted).
5. Return sample in 2. as the drawed sample.

When calculate pdf of a sample:
1. Calculate both pdf's probablity of the sample.
2. Sum them up, divide it by 2.0f, return as the probablity of the sample.

So we could use the GGX specular sampling method and Lambert sampling method at the same time.

Also again, this is only required for dieletric materal. For metal, it doesn't have diffuse, and for TransmittanceDieletric, we assume all lights are refracted or reflected, and none is diffused.
*/

/*
For Transmittance dieletric, when a ray hits it, the ray could be reflected or refracted.
In path tracer, only a single ray could be furthur traced.  
So in bsdf sampling, we chose to reflect the ray or refract the ray, based on fresnel strength of the microsurface with light direction.  
*/

/*Given sample represented as w_o, n, w_i, calculate probablity of it.*/
float Material::BSDFSampler::pdf(Vector3f x, Vector3f w_o, Vector3f n, Vector3f w_i) {

	float nv = dotProduct(n, w_o), nl = dotProduct(n, w_i);
	//Get the fresnel.
	Vector3f h = GetHalfDir(n, w_i, w_o, mat->ior_d);
	Vector3f f = mat->fresnel(w_o, h);
	//Calculate probablity of h.
	float pdf_h = GGXHalfPDF(n, h, mat->rough);

	float vh = dotProduct(w_o, h);
	float abs_vh = abs(vh);
	float lh = dotProduct(w_i, h);
	float ior_i, ior_o;
	GetInsideOutsideIOR(n, w_i, w_o, mat->ior_d, ior_i, ior_o);
	if (nv * nl < 0.0f) {
		//Refraction.
		float den = (ior_i * lh + ior_o * vh);
		float jaco = ior_o * ior_o * abs_vh / (den * den);
		if (mat->m_type != TransmittanceDieletric)
			return 0.0f;

		return pdf_h * (1.0f - f.x) * jaco;
	}
	else if (nv * nl > 0.0f) {
		//Reflection
		float jaco = 1.0f / (4.0f * abs_vh);
		auto diffuse = getDiffusePdf(w_o, n, w_i);
		if (mat->m_type == Metal) {
			return pdf_h * jaco;
		}
		else if (mat->m_type == Dieletric) {
			return (diffuse + pdf_h * jaco) * 0.5f;
		}
		else {
			return pdf_h * f.x * jaco;
		}
	}
	else {
		return 0.0f;
	}
}

/*Given w_o, n, draw a w_i sample, return it and its pdf*/
Vector3f Material::BSDFSampler::sample(Vector3f x, Vector3f w_o, Vector3f n, float* pdf) {
	Vector3f H = SampleGGXSpecularH(n, mat->rough);
	Vector3f w_i_s = reflect(w_o, H);
	float pdf_h = GGXHalfPDF(n, H, mat->rough);

	float vn = dotProduct(w_o, n);
	float nh = dotProduct(n, H);
	float vh = dotProduct(w_o, H);
	float abs_vh = abs(vh);
	float ior_i, ior_o;
	float jaco_reflect = 1.0f / (4.0f * abs_vh);
	if (mat->m_type == Metal) {
		*pdf = pdf_h * jaco_reflect;
		if (vn * dotProduct(w_i_s, n) < 0.0f)
			*pdf = 0.0f;
		return w_i_s;
	}
	else if (mat->m_type == Dieletric) {
		if (get_random_float() < 0.5f) {
			//Specular.
			float pdf_d = getDiffusePdf(w_o, n, w_i_s);
			*pdf = (pdf_h * jaco_reflect + pdf_d) * 0.5f;
			if (vn * dotProduct(w_i_s, n) < 0.0f)
				*pdf = 0.0f;
			return w_i_s;
		}
		else {
			//Diffuse.
			float pdf_d;
			Vector3f w_i_d = getDiffuseSampleAndPdf(w_o, n, pdf_d);
			H = (w_i_d + w_o).normalized();
			vh = dotProduct(w_o, H);
			abs_vh = abs(vh);
			pdf_h = GGXHalfPDF(n, H, mat->rough);
			jaco_reflect = 1.0f / (4.0f * abs_vh);
			if (vn * dotProduct(w_i_d, n) < 0.0f)
				*pdf = (pdf_h * jaco_reflect + pdf_d) * 0.5f;
			*pdf = 0.0f;
			return w_i_d;
		}
	}
	else {  //Transmittance dieletric.
		Vector3f f = mat->fresnel(w_o, H);

		if (get_random_float() < f.x) {
			//Go reflection.
			*pdf = pdf_h * f.x * jaco_reflect;
			if (vn * dotProduct(w_i_s, n) < 0.0f)
				*pdf = 0.0f;
			return w_i_s;
		}
		else {
			Vector3f w_i_refract = refract(w_o, H, mat->ior_d);
			GetInsideOutsideIOR(n, w_i_refract, w_o, mat->ior_d, ior_i, ior_o);
			float lh = dotProduct(w_i_refract, H);
			float den = (ior_i * lh + ior_o * vh);
			float jaco_refract = ior_o * ior_o * abs_vh / (den * den);
			//go Refraction
			*pdf = pdf_h * (1.0f - f.x) * jaco_refract;
			if (vn * dotProduct(w_i_refract, n) > 0.0f)
				*pdf = 0.0f;
			return w_i_refract;
		}
	}
}


void Material::SetSmoothness(float smooth) {
	this->rough = SmoothnessToRoughenss(smooth);
}

Vector3f Material::fresnel(Vector3f I, const Vector3f& N) const {
	if (this->m_type == Metal) {
		float cosTheta = dotProduct(I, N);
		float cosTheta2 = cosTheta * cosTheta;
		float sinTheta2 = 1.0 - cosTheta2;
		Vector3f TwoEtaCosTheta = 2.0 * ior_m * cosTheta;
		Vector3f t0 = ior_m * ior_m + ior_m_k * ior_m_k;
		Vector3f t1 = t0 * cosTheta2;
		Vector3f Rs = (t0 - TwoEtaCosTheta + cosTheta2) / (t0 + TwoEtaCosTheta + cosTheta2);
		Vector3f Rp = (t1 - TwoEtaCosTheta + 1.0) / (t1 + TwoEtaCosTheta + 1);
		return 0.5f * (Rp + Rs);
	}
	else {
		I = -I;
		float cosi = clamp(-1, 1, dotProduct(I, N));
		float etai = 1, etat = this->ior_d;
		if (cosi > 0) { std::swap(etai, etat); }
		// Compute sini using Snell's law
		float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
		// Total internal reflection
		if (sint >= 1) {
			return 1;
		}
		else {
			float cost = sqrtf(std::max(0.f, 1 - sint * sint));
			cosi = fabsf(cosi);
			float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
			float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
			return (Rs * Rs + Rp * Rp) / 2;
		}
	}
}

Material::Material(MaterialType t, Vector3f e) {
	m_type = t;
	//m_color = c;
	m_emission = e;
	Kd = Vector3f(0.5f, 0.5f, 0.5f);
}

MaterialType Material::getType() { return m_type; }

Vector3f Material::getEmission() { return m_emission; }

bool Material::hasEmission() {
	if (m_emission.x > 0.0f || m_emission.y > 0.0f || m_emission.z > 0.0f) return true;
	else return false;
}
