#pragma once

#include "Vec3.h"

class OutputImage;
class Ray;
class InputListener;
class SimpleCamera;
class World;

class HomemadeRayTracer
{
public:
	HomemadeRayTracer(InputListener *inputListener, OutputImage *image, const World *world);
	~HomemadeRayTracer();

	void						OnInit();
	void						OnUpdate(const SimpleCamera *camera, OutputImage *image);
	void						OnDestroy();
	void						HelpInfo();

	void						TraceRay(const SimpleCamera *camera, OutputImage *image);

private:
	Vec3						Sample(const Ray &r, UINT32 depth) const;

	InputListener *				m_inputListener{ nullptr };

	BOOL						m_enableNormalDisplay{ FALSE };
	BOOL						m_enable1SPP{ TRUE };

	const World *				m_world{ nullptr };
};
