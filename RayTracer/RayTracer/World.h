#pragma once

class Material;
class Hitable;

class World
{
public:
	void						ConstructWorld();
	void						DeconstructWorld();
	const Hitable *				GetRootHitable() const { return m_rootHitable; }

private:
	Hitable *					m_rootHitable{ nullptr };

	std::vector<Material *>		m_materialList;
	std::vector<Hitable *>		m_hitableList;
};