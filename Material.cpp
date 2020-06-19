#include "Material.hpp"
#include "SampleHelperFunctions.hpp"
#include "GGX.hpp"

/*
Given w_i, w_o, N, calculate how much light would be passed per unit solid angle.
Note that return value is not BSDF, since we combine the cosine-term( dot(normal, light) ) calculation here.

The code is just an plain implementation of Walter07. See the paper for why and how.
*/
Vector3f Material::evalGivenSample(const Vector3f& wo, const Vector3f& wi, const Vector3f& N, bool combineCosineTerm) {
	float nl = DotProduct(N, wi);
	float nv = DotProduct(N, wo);
	if (nl == 0.0f || nv == 0.0f)
		return 0.0f;
	Vector3f h = GetHalfDir(N, wi, wo, ior_d);

	float nh = DotProduct(N, h);
	float lv = DotProduct(wi, wo);
	float lh = DotProduct(wi, h);
	float vh = DotProduct(wo, h);
	float D = GGXTerm(nh, rough);
	float G = Visibility(nv, vh, rough) * Visibility(nl, lh, rough);
	Vector3f f = fresnel(wi, h);

	if (nl * nv > 0.0f)      //Reflection.
	{
		Vector3f specular = 0;
		if (G != 0.0f)
		{
			specular = (D * f * G) / (4.0 * abs(nv));       //Original formula is DFG/(4*nv*nl), we combine the cosine term here, so nl canceld out with it.
			if (!combineCosineTerm) {
				specular = specular / abs(nl);
			}
		}

		Vector3f diffuse = 0;
		if (this->m_type == Dieletric) {
			diffuse = this->Kd * (Vector3f::One() - f) / M_PI;
			if (combineCosineTerm) {
				diffuse = diffuse * saturate(nl);
			}
		}

		return diffuse + specular;
	}
	else {      //Refraction
		//Calculate half dir.
		if (m_type != Transparent)
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
		if (!combineCosineTerm) {
			partA /= abs(nl);
		}

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
It's possible to create another material dedicated for diffuse, using Lambert.
But if we want smooth dieletric(plastic pipe etc.), it's combined with diffuse and specular.

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
float Material::pdf(Vector3f w_o, Vector3f n, Vector3f w_i) {

	float nv = DotProduct(n, w_o), nl = DotProduct(n, w_i);
	if (nv == 0.0f || nl == 0.0f)
		return 0.0f;
	//Get the fresnel.
	Vector3f h = GetHalfDir(n, w_i, w_o, ior_d);
	Vector3f f = fresnel(w_o, h);
	//Calculate probablity of h.
	float pdf_h = GGXHalfPDF(n, h, rough);

	float vh = DotProduct(w_o, h);
	float abs_vh = abs(vh);
	float lh = DotProduct(w_i, h);
	float ior_i, ior_o;
	GetInsideOutsideIOR(n, w_i, w_o, ior_d, ior_i, ior_o);
	if (nv * nl < 0.0f) {
		//Refraction.
		float den = (ior_i * lh + ior_o * vh);
		float jaco = SafeDivide(ior_o * ior_o * abs_vh, (den * den));
		if (m_type != Transparent)
			return 0.0f;

		return pdf_h * (1.0f - f.x) * jaco;
	}
	else if (nv * nl > 0.0f) {
		//Reflection
		float jaco = SafeDivide(1.0f, (4.0f * abs_vh));
		auto diffuse = GetCosineWeightedPdf( n, w_i);
		if (m_type == Metal) {
			return pdf_h * jaco;
		}
		else if (m_type == Dieletric) {
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
Vector3f Material::sample(Vector3f w_o, Vector3f n, float* pdf) {
	Vector3f H = SampleGGXSpecularH(n, rough);
	Vector3f w_i_s = Reflect(w_o, H);
	float pdf_h = GGXHalfPDF(n, H, rough);

	float vn = DotProduct(w_o, n);
	float nh = DotProduct(n, H);
	float vh = DotProduct(w_o, H);
	float abs_vh = abs(vh);
	float ior_i, ior_o;
	float jaco_reflect = SafeDivide(1.0f, (4.0f * abs_vh));
	if (m_type == Metal) {
		*pdf = pdf_h * jaco_reflect;
		if (vn * DotProduct(w_i_s, n) < 0.0f)
			*pdf = 0.0f;
		return w_i_s;
	}
	else if (m_type == Dieletric) {
		if (GetRandomFloat() < 0.5f) {
			//Specular.
			float pdf_d = GetCosineWeightedPdf(n, w_i_s);
			*pdf = (pdf_h * jaco_reflect + pdf_d) * 0.5f;
			if (vn * DotProduct(w_i_s, n) < 0.0f)
				*pdf = 0.0f;
			return w_i_s;
		}
		else {
			//Diffuse.
			float pdf_d;
			Vector3f w_i_d = GetCosineWeightedSample(n, pdf_d);
			H = (w_i_d + w_o).Normalized();
			vh = DotProduct(w_o, H);
			abs_vh = abs(vh);
			pdf_h = GGXHalfPDF(n, H, rough);
			jaco_reflect = SafeDivide(1.0f, (4.0f * abs_vh));
			*pdf = (pdf_h * jaco_reflect + pdf_d) * 0.5f;
			if (vn * DotProduct(w_i_d, n) < 0.0f)
				*pdf = 0.0f;
			return w_i_d;
		}
	}
	else {  //Transmittance dieletric.
		Vector3f f = fresnel(w_o, H);

		if (GetRandomFloat() < f.x) {
			//Go reflection.
			*pdf = pdf_h * f.x * jaco_reflect;
			if (vn * DotProduct(w_i_s, n) < 0.0f)
				*pdf = 0.0f;
			return w_i_s;
		}
		else {
			Vector3f w_i_refract = Refract(w_o, H, ior_d);
			GetInsideOutsideIOR(n, w_i_refract, w_o, ior_d, ior_i, ior_o);
			float lh = DotProduct(w_i_refract, H);
			float den = (ior_i * lh + ior_o * vh);
			float jaco_refract = SafeDivide(ior_o * ior_o * abs_vh, (den * den));
			//go Refraction
			*pdf = pdf_h * (1.0f - f.x) * jaco_refract;
			if (vn * DotProduct(w_i_refract, n) > 0.0f)
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
		float cosTheta = DotProduct(I, N);
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
		float cosi = std::clamp(DotProduct(I, N), -1., 1.);
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
