#pragma once

#include "Vec3.h"

class OutputImage;
class Ray;
class InputListener;
class SimpleCamera;
class Hitable;

class HomemadeRayTracer
{
public:
	HomemadeRayTracer(InputListener *inputListener, OutputImage *image, const Hitable *world);
	~HomemadeRayTracer();

	void						OnInit();
	void						OnUpdate(const SimpleCamera *camera, OutputImage *image);
	void						OnDestroy();
	void						HelpInfo();

	void						Render(const SimpleCamera *camera, OutputImage *image);

private:
	Vec3						Sample(const Ray &r, UINT32 depth) const;

	InputListener *				m_inputListener{ nullptr };

	BOOL						m_enableNormalDisplay{ FALSE };
	BOOL						m_enblaeAA{ TRUE };

	const Hitable *				m_world{ nullptr };
};
