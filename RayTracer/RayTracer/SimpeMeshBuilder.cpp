#include "stdafx.h"
#include "SimpeMeshBuilder.h"

#include "SimpleMesh.h"

#define BUILD_MESH_OPTIMIZE_FOR_REUSE 14

void SimpeMeshBuilder::BuildTorusMesh(BuildMeshMode const eMode, SimpleMesh *destMesh, float outerRadius, float innerRadius, UINT16 outerQuads, UINT16 innerQuads, float outerRepeats, float innerRepeats)
{
	assert(outerQuads >= 3);
	assert(innerQuads >= 3);

	const UINT32 outerVertices = outerQuads + 1;
	const UINT32 innerVertices = innerQuads + 1;
	const UINT32 vertices = outerVertices * innerVertices;
	// If we can prime the reuse cache with the first row of vertices, the maximum number of vertices we issue as a strip 
	// before we have to jump back to a parallel strip is (BUILD_MESH_OPTIMIZE_FOR_REUSE - 1).  
	// For a 14 index reuse buffer, the reuse cache index used by each index looks like this:
	//  0--1--2--3--4--5--6--7--8--9-10-11-12 <- Primed with 7 degenerate triangles: {0,0,1}, {2,2,3}, {4,4,5}, {6,6,7}, {8,8,9}, {10,10,11}, {12,12,12} 
	//  |/ |/ |/ |/ |/ |/ |/ |/ |/ |/ |/ |/ |
	// 13--0--1--2--3--4--5--6--7--8--9-10-11 <- Note that starting with the second triangle, the index added by each triangle pushes an index out of the cache that was just used for the last time by the previous triangle.
	//  |/ |/ |/ |/ |/ |/ |/ |/ |/ |/ |/ |/ |    
	// 12-13--0--1--2--3--4--5--6--7--8--9-10 <- In each row, there are 12 quads or 24 triangles which add 13 new vertices.  The asymptotic reuse rate is 24/13 = ~1.846 prims per vertex.
	//  |/ |/ |/ |/ |/ |/ |/ |/ |/ |/ |/ |/ |    
	//  <-----------------------------------> We call this a "stripe" - a stack of parallel rows - a "full stripe" if it contains the number of quads which maximizes reuse, and a "maximal stripe" if it maximizes reuse with cache priming.
	const UINT32 maxQuadsInStripeForReuseWithPrimedCache = (BUILD_MESH_OPTIMIZE_FOR_REUSE - 2);
	// Dispatch draw prevents us from priming the reuse cache, because at any point during a stripe, dispatch draw might
	// cull a series of triangles, leaving the cache unprimed again.  Once the cache becomes unprimed, stripes of this
	// maximal width revert to the reuse rate of a triangle strip, about 1.0 prims per vertex:
	//  0--2--4--6--8-10-12--0--2--4--6--8-10 <- without priming the cache, we get a strip-like pattern in the reuse buffer
	//  |/ |/ |/ |/ |/ |/ |/ |/ |/ |/ |/ |/ |
	//  1--3--5--7--9-11-13--1--3--5--7--9-11
	// 12--0--2--4--6--8-10-12--0--2--4--6--8 <- Note that no reuse occurs against the adjacent strip, because the adjacent indices are all overwritten
	//  |/ |/ |/ |/ |/ |/ |/ |/ |/ |/ |/ |/ |    In each row, there are now 24 triangles which add 26 new vertices, dropping the reuse rate to ~0.923 prims per vertex.
	// 13--1--3--5--7--9-11-13--1--3--5--7--9
	// The maximum stripe width which works without priming the reuse cache is (BUILD_MESH_OPTIMIZE_FOR_REUSE - 2)/2 + 1 vertices:
	//  0--2--4--6--8-10-12 <- without priming the cache, we get a strip-like pattern in the reuse buffer
	//	|/ |/ |/ |/ |/ |/ |
	//	1--3--5--7--9-11-13
	//  |/ |/ |/ |/ |/ |/ |
	//  0--1--2--3--4--5--6 <- Again, starting with the second triangle, the index added by each triangle pushes an index out of the cache that was just used for the last time by the previous triangle.
	//  |/ |/ |/ |/ |/ |/ |
	//  7--8--9-10-11-12-13 <- In each row, there are 6 quads or 12 triangles which add 7 new vertices.  The asymptotic reuse rate is 12/7 = ~1.714 prims per vertex.
	//  |/ |/ |/ |/ |/ |/ |
	//  <-----------------> A "full stripe" for dispatch draw has (BUILD_MESH_OPTIMIZE_FOR_REUSE - 2)/2 quads.
	const UINT32 innerQuadsFullStripe =
		(eMode >= kBuildDispatchDrawOptimizedIndices) ? maxQuadsInStripeForReuseWithPrimedCache / 2 :	// dispatch draw prevents priming the reuse cache by culling triangles; optimize by traversing in the widest parallel strips that will achieve 0.5 new vertices for most triangles without priming the reuse cache. 
		(eMode >= kBuildOptimizedIndices) ? maxQuadsInStripeForReuseWithPrimedCache :				// optimize by traversing in the widest parallel stripes that will achieve 0.5 new vertices for most triangles, assuming we can prime the reuse cache.
		innerQuads;																					// just traverse the full innerQuads width of the torus.
																									// To stripe the entire torus which has innerQuads x outerQuads quads into N stripes, the last of which may be less than a full stripe width:
	const UINT32 numInnerQuadsFullStripes = innerQuads / innerQuadsFullStripe;
	const UINT32 innerQuadsLastStripe = innerQuads - numInnerQuadsFullStripes * innerQuadsFullStripe;
	// I won't get into the details, here, but it turns out that priming a stripe that is smaller than "maximal" but
	// too large to work without priming takes (stripeWidth - (maximalStripeWidth-1)/2) degenerate triangles.
	// For a reuse cache of 14 indices and stripes of width 12 quads, it takes 7 triangles.  
	// For 11 quads -> 6 tris, 10 quads -> 5 tris, ..., 7 quads -> 2 tris, and for <=6 quads, no priming is necessary.
	const UINT32 trianglesToPrimeFullStripe = (eMode >= kBuildOptimizedIndices && innerQuadsFullStripe > maxQuadsInStripeForReuseWithPrimedCache / 2) ? innerQuadsFullStripe - (maxQuadsInStripeForReuseWithPrimedCache - 1) / 2 : 0;
	const UINT32 trianglesToPrimeLastStripe = (innerQuadsLastStripe > maxQuadsInStripeForReuseWithPrimedCache / 2) ? innerQuadsLastStripe - (maxQuadsInStripeForReuseWithPrimedCache - 1) / 2 : 0;
	const UINT32 triangles = 2 * outerQuads * innerQuads + trianglesToPrimeFullStripe * numInnerQuadsFullStripes + trianglesToPrimeLastStripe;

	//TODO: Dispatch draw must also break up the index data into blocks for processing by compute, 
	// each of which contains no more than 256 vertices and 512 triangles.
	// Minimizing the number of index blocks is also important, and can be achieved by outputting
	// patches of 256 vertices which minimize the number of vertices shared with other patches.
	// Minimizing index blocks will necessarily trade off against maximizing vertex reuse.
	// Simple striping of width 6 quads results in index blocks with:
	// (7 x 36 + 4) vertices ~= (6 x 35 + [2:3] = [212:213]) quads = [424:426] triangles, with [84:86] vertices shared, 4 shared 4 ways.
	// This has an asymptotic unculled vertex reuse rate of ~1.704.
	//
	// The best repetitive pattern is to output square patches of 16 x 16 vertices with 
	// (16-1) x (16-1) = 225 quads or 450 triangles with 60 vertices shared with adjacent patches, 4 shared 4 ways.
	// This results in ~6% fewer index blocks but reduces the asymptotic unculled vertex reuse rate to ~1.601 prims per vertex (6.4% more vertices),
	// assuming we always continue the last full stripe from the previous block into the next block.
	//
	// As this isn't a multiple of our best 6 quad stripe width, we probably instead want to use exactly
	// 2 stripes (13 x 19 + 9) vertices ~= (12 x 18 + [7:8] = [223:224]) quads = [446:448] triangles, with [62:64] vertices shared, 4 shared 4 ways.
	// This results in ~5% fewer index blocks but reduces the asymptotic unculled vertex reuse rate to ~1.660 prims per vertex (2.6% more vertices),
	// assuming we always continue the last full stripe from the previous block into the next block.
	//
	// When tiling a finite plane, there are patterns better than the best repetitive pattern, due
	// to the fact that plane edge vertices aren't reused and because our repetitive block width
	// may not divide evenly into the plane width.

	if (eMode & 0x1) {		//BuildVertices
		destMesh->m_vertexCount = vertices;
		destMesh->m_vertexStride = sizeof(SimpleMeshVertex);
		destMesh->m_vertexAttributeCount = 5;
		destMesh->m_vertexBufferSize = destMesh->m_vertexCount * destMesh->m_vertexStride;
	}
	if (eMode > kBuildVerticesOnly) {	//BuildIndices
		destMesh->m_indexCount = triangles * 3;
		destMesh->m_indexType = kIndexSize16;
		destMesh->m_primitiveType = kPrimitiveTypeTriList;
		destMesh->m_indexBufferSize = destMesh->m_indexCount * sizeof(UINT16);
	}

	if (eMode & 0x1) {		//BuildVertices
		destMesh->m_vertexBuffer = new UINT8[destMesh->m_vertexBufferSize];
		if (destMesh->m_vertexBuffer == NULL) {
			assert(destMesh->m_vertexBuffer != NULL);
			return;
		}

		SimpleMeshVertex *outV = static_cast<SimpleMeshVertex*>(destMesh->m_vertexBuffer);
		const XMFLOAT2 textureScale(outerRepeats / (outerVertices - 1), innerRepeats / (innerVertices - 1));
		for (UINT32 o = 0; o < outerVertices; ++o)
		{
			const float outerTheta = o * 2 * (float)M_PI / (outerVertices - 1);
			const XMMATRIX outerToWorld = DirectX::XMMatrixRotationZ(outerTheta) * DirectX::XMMatrixTranslation(outerRadius, 0, 0);
			for (UINT32 i = 0; i < innerVertices; ++i)
			{
				const float innerTheta = i * 2 * (float)M_PI / (innerVertices - 1);
				const XMMATRIX innerToOuter = DirectX::XMMatrixRotationY(innerTheta) * DirectX::XMMatrixTranslation(innerRadius, 0, 0);
				const XMMATRIX localToWorld = outerToWorld * innerToOuter;

				// outV->m_position = localToWorld * XMVECTOR(0, 0, 0, 1);
				XMVECTOR v = DirectX::XMVectorSet(0, 0, 0, 1);
				DirectX::XMStoreFloat3(&outV->m_position, DirectX::XMVector4Transform(v, localToWorld));
				
				//outV->m_normal = localToWorld * XMVECTOR(1, 0, 0, 0);
				v = DirectX::XMVectorSet(1, 0, 0, 0);
				DirectX::XMStoreFloat3(&outV->m_normal, DirectX::XMVector4Transform(v, localToWorld));

				//outV->m_tangent = localToWorld * XMVECTOR(0, 1, 0, 0);
				v = DirectX::XMVectorSet(0, 1, 0, 0);
				DirectX::XMStoreFloat4(&outV->m_tangent, DirectX::XMVector4Transform(v, localToWorld));

				outV->m_texture = XMFLOAT2((float)o * textureScale.x, (float)i * textureScale.y);
				++outV;
			}
		}
		assert(outV == static_cast<SimpleMeshVertex*>(destMesh->m_vertexBuffer) + vertices);
	}

	if (eMode > kBuildVerticesOnly) {	//BuildIndices
		destMesh->m_indexBuffer = new UINT8[destMesh->m_indexBufferSize];
		if (destMesh->m_indexBuffer == NULL) {
			assert(destMesh->m_indexBuffer != NULL);
			return;
		}

		UINT16 *outI = static_cast<UINT16*>(destMesh->m_indexBuffer);
		UINT16 const numInnerQuadsStripes = numInnerQuadsFullStripes + (innerQuadsLastStripe > 0 ? 1 : 0);
		for (UINT16 iStripe = 0; iStripe < numInnerQuadsStripes; ++iStripe)
		{
			UINT16 const innerVertex0 = iStripe * innerQuadsFullStripe;
			UINT16 const trianglesToPrimeStripe = (iStripe < numInnerQuadsFullStripes ? trianglesToPrimeFullStripe : trianglesToPrimeLastStripe);
			UINT16 const innerQuadsStripe = (iStripe < numInnerQuadsFullStripes ? innerQuadsFullStripe : innerQuadsLastStripe);
			// prime full strip with degenerate triangles that inject { 0, 1, 2, 3, ... } into the reuse cache
			for (UINT16 iPrime = 0; iPrime < trianglesToPrimeStripe; ++iPrime) {
				outI[0] = innerVertex0 + iPrime * 2;
				outI[1] = innerVertex0 + iPrime * 2;
				outI[2] = innerVertex0 + iPrime * 2 + 1;
				outI += 3;
			}
			if (trianglesToPrimeStripe > 0 && outI[-1] > innerVertex0 + innerQuadsFullStripe)
				outI[-1] = innerVertex0 + innerQuadsFullStripe;	// if a full strip has an even number of quads, we have to prime an odd number of vertices, and the last degenerate has only one vertex to prime
																// emit full strip 
			for (UINT16 o = 0; o < outerQuads; ++o)
			{
				for (UINT16 i = 0; i < innerQuadsStripe; ++i)
				{
					const UINT16 index[4] =
					{
						static_cast<UINT16>((o + 0) * innerVertices + innerVertex0 + (i + 0)),
						static_cast<UINT16>((o + 0) * innerVertices + innerVertex0 + (i + 1)),
						static_cast<UINT16>((o + 1) * innerVertices + innerVertex0 + (i + 0)),
						static_cast<UINT16>((o + 1) * innerVertices + innerVertex0 + (i + 1)),
					};
					outI[0] = index[0];
					outI[1] = index[1];
					outI[2] = index[2];
					outI[3] = index[2];
					outI[4] = index[1];
					outI[5] = index[3];
					outI += 6;
				}
			}
		}
		assert(outI == static_cast<UINT16*>(destMesh->m_indexBuffer) + triangles * 3);
	}
}

void SimpeMeshBuilder::BuildSphereMesh(SimpleMesh *destMesh, float radius, long xdiv, long ydiv, float tx, float ty, float tz)
{
	destMesh->m_vertexCount = (xdiv + 1) * (ydiv + 1);

	destMesh->m_vertexStride = sizeof(SimpleMeshVertex);

	destMesh->m_vertexAttributeCount = 5;
	destMesh->m_indexCount = (xdiv * (ydiv - 1) * 2) * 3;
	destMesh->m_indexType = kIndexSize16;
	destMesh->m_primitiveType = kPrimitiveTypeTriList;

	destMesh->m_vertexBufferSize = destMesh->m_vertexCount * sizeof(SimpleMeshVertex);
	destMesh->m_indexBufferSize = destMesh->m_indexCount * sizeof(UINT16);

	destMesh->m_vertexBuffer = new UINT8[destMesh->m_vertexBufferSize];
	destMesh->m_indexBuffer = new UINT8[destMesh->m_indexBufferSize];

	memset(destMesh->m_vertexBuffer, 0, destMesh->m_vertexBufferSize);
	memset(destMesh->m_indexBuffer, 0, destMesh->m_indexBufferSize);

	// Everything else is just filling in the vertex and index buffer.
	SimpleMeshVertex *outV = static_cast<SimpleMeshVertex*>(destMesh->m_vertexBuffer);
	UINT16 *outI = static_cast<UINT16*>(destMesh->m_indexBuffer);

	const float gx = 2 * (float)M_PI / xdiv;
	const float gy = (float)M_PI / ydiv;

	for (long i = 0; i < xdiv; ++i)
	{
		const float theta = (float)i * gx;
		const float ct = cosf(theta);
		const float st = sinf(theta);

		const long k = i * (ydiv + 1);
		for (long j = 1; j < ydiv; ++j)
		{
			const float phi = (float)j * gy;
			const float sp = sinf(phi);
			const float x = ct * sp;
			const float y = st * sp;
			const float z = cosf(phi);

			outV[k + j].m_position = XMFLOAT3(x*radius + tx, y*radius + ty, z*radius + tz);
			outV[k + j].m_normal = XMFLOAT3(x, y, z);
			outV[k + j].m_texture = XMFLOAT2(theta * 0.1591549430918953f, phi * 0.31830988618379f);
		}
	}

	const long kk = xdiv * (ydiv + 1);
	for (long j = 1; j < ydiv; ++j)
	{
		const float phi = (float)j * gy;
		const float x = sinf(phi);
		const float z = cosf(phi);

		outV[kk + j].m_position = XMFLOAT3(x*radius + tx, ty, z*radius + tz);
		outV[kk + j].m_normal = XMFLOAT3(x, 0, z);
		outV[kk + j].m_texture = XMFLOAT2(1, phi * 0.31830988618379f);

	}

	for (long i = 0; i < xdiv; i++)
	{
		const long k1 = i * (ydiv + 1) + 1;
		const long k2 = (i + 1) * (ydiv + 1) + 1;
		const float s = (outV[k1].m_texture.x + outV[k2].m_texture.x) * 0.5f;

		outV[k1 - 1].m_position = XMFLOAT3(tx, ty, radius + tz);
		outV[k1 - 1].m_normal = XMFLOAT3(0, 0, 1);
		outV[k1 - 1].m_texture = XMFLOAT2(s, 0);


		outV[k1 + ydiv - 1].m_position = XMFLOAT3(tx, ty, -radius + tz);
		outV[k1 + ydiv - 1].m_normal = XMFLOAT3(0, 0, -1);
		outV[k1 + ydiv - 1].m_texture = XMFLOAT2(s, 1);

	}

	outV[xdiv*(ydiv + 1)].m_position = outV[0].m_position;
	outV[xdiv*(ydiv + 1)].m_normal = outV[0].m_normal;
	outV[xdiv*(ydiv + 1)].m_texture = outV[0].m_texture;

	outV[xdiv*(ydiv + 1) + ydiv].m_position = outV[ydiv].m_position;
	outV[xdiv*(ydiv + 1) + ydiv].m_normal = outV[ydiv].m_normal;
	outV[xdiv*(ydiv + 1) + ydiv].m_texture = outV[ydiv].m_texture;

	long ii = 0;
	for (long i = 0; i < xdiv; ++i)
	{
		const long k = i * (ydiv + 1);

		outI[ii + 0] = (UINT16)k;
		outI[ii + 1] = (UINT16)(k + 1);
		outI[ii + 2] = (UINT16)(k + ydiv + 2);
		ii += 3;

		for (long j = 1; j < ydiv - 1; ++j)
		{
			outI[ii + 0] = (UINT16)(k + j);
			outI[ii + 1] = (UINT16)(k + j + 1);
			outI[ii + 2] = (UINT16)(k + j + ydiv + 2);
			outI[ii + 3] = (UINT16)(k + j);
			outI[ii + 4] = (UINT16)(k + j + ydiv + 2);
			outI[ii + 5] = (UINT16)(k + j + ydiv + 1);
			ii += 6;
		}

		outI[ii + 0] = (UINT16)(k + ydiv - 1);
		outI[ii + 1] = (UINT16)(k + ydiv);
		outI[ii + 2] = (UINT16)(k + ydiv * 2);
		ii += 3;
	}

	// Double texcoords
	for (UINT32 i = 0; i < destMesh->m_vertexCount; ++i)
	{
		outV[i].m_texture = XMFLOAT2(outV[i].m_texture.x * 4.f, outV[i].m_texture.y * 2.f);
	}

	// Calculate tangents
	XMFLOAT3* tan1 = new XMFLOAT3[destMesh->m_vertexCount];
	XMFLOAT3* tan2 = new XMFLOAT3[destMesh->m_vertexCount];

	memset(tan1, 0, sizeof(XMFLOAT3)*destMesh->m_vertexCount);
	memset(tan2, 0, sizeof(XMFLOAT3)*destMesh->m_vertexCount);

	for (UINT32 i = 0; i < destMesh->m_indexCount / 3; ++i)
	{
		const long i1 = outI[i * 3 + 0];
		const long i2 = outI[i * 3 + 1];
		const long i3 = outI[i * 3 + 2];
		const XMFLOAT3 v1 = outV[i1].m_position;
		const XMFLOAT3 v2 = outV[i2].m_position;
		const XMFLOAT3 v3 = outV[i3].m_position;
		const XMFLOAT2 w1 = outV[i1].m_texture;
		const XMFLOAT2 w2 = outV[i2].m_texture;
		const XMFLOAT2 w3 = outV[i3].m_texture;

		const float x1 = v2.x - v1.x;
		const float x2 = v3.x - v1.x;
		const float y1 = v2.y - v1.y;
		const float y2 = v3.y - v1.y;
		const float z1 = v2.z - v1.z;
		const float z2 = v3.z - v1.z;

		const float s1 = w2.x - w1.x;
		const float s2 = w3.x - w1.x;
		const float t1 = w2.y - w1.y;
		const float t2 = w3.y - w1.y;

		const float r = 1.f / (s1*t2 - s2 * t1);
		const XMFLOAT3 sdir((t2*x1 - t1 * x2)*r, (t2*y1 - t1 * y2)*r, (t2*z1 - t1 * z2)*r);
		const XMFLOAT3 tdir((s1*x2 - s2 * x1)*r, (s1*y2 - s2 * y1)*r, (s1*z2 - s2 * z1)*r);

		
		tan1[i1] = XMFLOAT3(tan1[i1].x + sdir.x, tan1[i1].y + sdir.y, tan1[i1].z + sdir.z);
		tan1[i2] = XMFLOAT3(tan1[i2].x + sdir.x, tan1[i2].y + sdir.y, tan1[i2].z + sdir.z); 
		tan1[i3] = XMFLOAT3(tan1[i3].x + sdir.x, tan1[i3].y + sdir.y, tan1[i3].z + sdir.z); 
		tan2[i1] = XMFLOAT3(tan1[i1].x + tdir.x, tan1[i1].y + tdir.y, tan1[i1].z + tdir.z); 
		tan2[i2] = XMFLOAT3(tan1[i2].x + tdir.x, tan1[i2].y + tdir.y, tan1[i2].z + tdir.z); 
		tan2[i3] = XMFLOAT3(tan1[i3].x + tdir.x, tan1[i3].y + tdir.y, tan1[i3].z + tdir.z);
	}
	const long count = destMesh->m_vertexCount;
	for (long i = 0; i < count; ++i)
	{
		const XMFLOAT3 n = outV[i].m_normal;
		const XMFLOAT3 t = tan1[i];
		const float nDotT = n.x * t.x + n.y * t.y + n.z * t.z;
		const XMFLOAT3 tan_a(t.x - n.x * nDotT, t.y - n.y * nDotT, t.z - n.z * nDotT);
		const float ooLen = 1.f / sqrtf(tan_a.x * tan_a.x + tan_a.y * tan_a.y + tan_a.z * tan_a.z);
		outV[i].m_tangent = XMFLOAT4(tan_a.x * ooLen, tan_a.y * ooLen, tan_a.z * ooLen, 1);
	}

	delete[] tan1;
	delete[] tan2;
}

void SimpeMeshBuilder::BuildQuadMesh(SimpleMesh *destMesh, float size)
{
	destMesh->m_vertexCount = 4;

	destMesh->m_vertexStride = sizeof(SimpleMeshVertex);

	destMesh->m_vertexAttributeCount = 5;
	destMesh->m_indexCount = 6;
	destMesh->m_indexType = kIndexSize16;
	destMesh->m_primitiveType = kPrimitiveTypeTriList;

	destMesh->m_vertexBufferSize = destMesh->m_vertexCount * sizeof(SimpleMeshVertex);
	destMesh->m_indexBufferSize = destMesh->m_indexCount * sizeof(UINT16);


	destMesh->m_vertexBuffer = new UINT8[destMesh->m_vertexBufferSize];
	destMesh->m_indexBuffer = new UINT8[destMesh->m_indexBufferSize];


	memset(destMesh->m_vertexBuffer, 0, destMesh->m_vertexBufferSize);
	memset(destMesh->m_indexBuffer, 0, destMesh->m_indexBufferSize);

	// Everything else is just filling in the vertex and index buffer.
	SimpleMeshVertex *outV = static_cast<SimpleMeshVertex*>(destMesh->m_vertexBuffer);
	UINT16 *outI = static_cast<UINT16*>(destMesh->m_indexBuffer);

	size *= 0.5f;
	const SimpleMeshVertex verts[4] =
	{
		{ { -size, -size, 0 },{ 0,0,1 },{ 1,0,0,1 },{ 0,1 } },
	{ { size, -size, 0 },{ 0,0,1 },{ 1,0,0,1 },{ 1,1 } },
	{ { -size,  size, 0 },{ 0,0,1 },{ 1,0,0,1 },{ 0,0 } },
	{ { size,  size, 0 },{ 0,0,1 },{ 1,0,0,1 },{ 1,0 } },
	};
	memcpy(outV, verts, 4 * sizeof(SimpleMeshVertex));

	outI[0] = 0;
	outI[1] = 1;
	outI[2] = 2;
	outI[3] = 1;
	outI[4] = 3;
	outI[5] = 2;
}

void SimpeMeshBuilder::BuildCubeMesh(SimpleMesh *destMesh, float side)
{
	destMesh->m_vertexCount = 24;

	destMesh->m_vertexStride = sizeof(SimpleMeshVertex);

	destMesh->m_vertexAttributeCount = 5;
	destMesh->m_indexCount = 36;
	destMesh->m_indexType = kIndexSize16;
	destMesh->m_primitiveType = kPrimitiveTypeTriList;

	destMesh->m_vertexBufferSize = destMesh->m_vertexCount * sizeof(SimpleMeshVertex);
	destMesh->m_indexBufferSize = destMesh->m_indexCount * sizeof(UINT16);

	destMesh->m_vertexBuffer = new UINT8[destMesh->m_vertexBufferSize];
	destMesh->m_indexBuffer = new UINT8[destMesh->m_indexBufferSize];

	memset(destMesh->m_vertexBuffer, 0, destMesh->m_vertexBufferSize);
	memset(destMesh->m_indexBuffer, 0, destMesh->m_indexBufferSize);

	// Everything else is just filling in the vertex and index buffer.
	SimpleMeshVertex *outV = static_cast<SimpleMeshVertex*>(destMesh->m_vertexBuffer);
	UINT16 *outI = static_cast<UINT16*>(destMesh->m_indexBuffer);

	const float halfSide = side * 0.5f;
	const SimpleMeshVertex verts[] = {
		{ { -halfSide, -halfSide, -halfSide },{ -1.0000000, 0.00000000, 0.00000000 },{ 0.00000000, 0.00000000, 1.0000000, 1.0000000 },{ 0, 0 } }, // 0
	{ { -halfSide, -halfSide, -halfSide },{ 0.00000000, -1.0000000, 0.00000000 },{ 1.0000000, 0.00000000, 0.00000000, 1.0000000 },{ 0, 0 } }, // 1
	{ { -halfSide, -halfSide, -halfSide },{ 0.00000000, 0.00000000, -1.0000000 },{ 1.0000000, 0.00000000, 0.00000000, 1.0000000 },{ 1, 0 } }, // 2
	{ { -halfSide, -halfSide, halfSide },{ -1.0000000, 0.00000000, 0.00000000 },{ 0.00000000, 0.00000000, 1.0000000, 1.0000000 },{ 0, 1 } }, // 3
	{ { -halfSide, -halfSide, halfSide },{ 0.00000000, -1.0000000, 0.00000000 },{ 1.0000000, 0.00000000, 0.00000000, 1.0000000 },{ 1, 0 } }, // 4
	{ { -halfSide, -halfSide, halfSide },{ 0.00000000, 0.00000000, 1.0000000 },{ 1.0000000, 0.00000000, 0.00000000, 1.0000000 },{ 0, 0 } }, // 5
	{ { -halfSide, halfSide, -halfSide },{ -1.0000000, 0.00000000, 0.00000000 },{ 0.00000000, 0.00000000, 1.0000000, 1.0000000 },{ 1, 0 } }, // 6
	{ { -halfSide, halfSide, -halfSide },{ 0.00000000, 0.00000000, -1.0000000 },{ 1.0000000, 0.00000000, 0.00000000, 1.0000000 },{ 0, 0 } }, // 7
	{ { -halfSide, halfSide, -halfSide },{ 0.00000000, 1.0000000, 0.00000000 },{ 1.0000000, 0.00000000, 0.00000000, 1.0000000 },{ 1, 0 } }, // 8
	{ { -halfSide, halfSide, halfSide },{ -1.0000000, 0.00000000, 0.00000000 },{ 0.00000000, 0.00000000, 1.0000000, 1.0000000 },{ 1, 1 } }, // 9
	{ { -halfSide, halfSide, halfSide },{ 0.00000000, 0.00000000, 1.0000000 },{ 1.0000000, 0.00000000, 0.00000000, 1.0000000 },{ 1, 0 } }, // 10
	{ { -halfSide, halfSide, halfSide },{ 0.00000000, 1.0000000, 0.00000000 },{ 1.0000000, 0.00000000, 0.00000000, 1.0000000 },{ 0, 0 } }, // 11
	{ { halfSide, -halfSide, -halfSide },{ 0.00000000, -1.0000000, 0.00000000 },{ 1.0000000, 0.00000000, 0.00000000, 1.0000000 },{ 0, 1 } }, // 12
	{ { halfSide, -halfSide, -halfSide },{ 0.00000000, 0.00000000, -1.0000000 },{ 1.0000000, 0.00000000, 0.00000000, 1.0000000 },{ 1, 1 } }, // 13
	{ { halfSide, -halfSide, -halfSide },{ 1.0000000, 0.00000000, 0.00000000 },{ 0.00000000, 0.00000000, -1.0000000, 1.0000000 },{ 0, 1 } }, // 14
	{ { halfSide, -halfSide, halfSide },{ 0.00000000, -1.0000000, 0.00000000 },{ 1.0000000, 0.00000000, 0.00000000, 1.0000000 },{ 1, 1 } }, // 15
	{ { halfSide, -halfSide, halfSide },{ 0.00000000, 0.00000000, 1.0000000 },{ 1.0000000, 0.00000000, 0.00000000, 1.0000000 },{ 0, 1 } }, // 16
	{ { halfSide, -halfSide, halfSide },{ 1.0000000, 0.00000000, 0.00000000 },{ 0.00000000, 0.00000000, -1.0000000, 1.0000000 },{ 0, 0 } }, // 17
	{ { halfSide, halfSide, -halfSide },{ 0.00000000, 0.00000000, -1.0000000 },{ 1.0000000, 0.00000000, 0.00000000, 1.0000000 },{ 0, 1 } }, // 18
	{ { halfSide, halfSide, -halfSide },{ 0.00000000, 1.0000000, 0.00000000 },{ 1.0000000, 0.00000000, 0.00000000, 1.0000000 },{ 1, 1 } }, // 19
	{ { halfSide, halfSide, -halfSide },{ 1.0000000, 0.00000000, 0.00000000 },{ 0.00000000, 0.00000000, -1.0000000, 1.0000000 },{ 1, 1 } }, // 20
	{ { halfSide, halfSide, halfSide },{ 0.00000000, 0.00000000, 1.0000000 },{ 1.0000000, 0.00000000, 0.00000000, 1.0000000 },{ 1, 1 } }, // 21
	{ { halfSide, halfSide, halfSide },{ 0.00000000, 1.0000000, 0.00000000 },{ 1.0000000, 0.00000000, 0.00000000, 1.0000000 },{ 0, 1 } }, // 22
	{ { halfSide, halfSide, halfSide },{ 1.0000000, 0.00000000, 0.00000000 },{ 0.00000000, 0.00000000, -1.0000000, 1.0000000 },{ 1, 0 } }, // 23
	};
	memcpy(outV, verts, sizeof(verts));

	const UINT16 indices[] = {
		5, 16, 10,		10, 16, 21,
		11, 22, 8,		8, 22, 19,
		7, 18, 2,		2, 18, 13,
		1, 12, 4,		4, 12, 15,
		17, 14, 23,		23, 14, 20,
		0, 3, 6,		6, 3, 9,
	};
	memcpy(outI, indices, sizeof(indices));
}

float SimpeMeshBuilder::ComputeMeshSpecificBumpScale(const SimpleMesh *srcMesh)
{
	float fMeshSpecificBumpScale = 1.0f;
	// sanity check
	if (srcMesh->m_primitiveType == kPrimitiveTypeTriList && sizeof(SimpleMeshVertex) == srcMesh->m_vertexStride)
	{
		const int nr_triangles = srcMesh->m_indexCount / 3;

		const SimpleMeshVertex * pfVerts = static_cast<const SimpleMeshVertex*>(static_cast<const void*>(srcMesh->m_vertexBuffer));
		const void * pIndices = static_cast<const void*>(srcMesh->m_indexBuffer);
		bool bIs32Bit = srcMesh->m_indexType == kIndexSize32;

		int iNrContributions = 0;
		double dAreaRatioSum = 0;
		for (int t = 0; t < nr_triangles; t++)
		{
			int i0 = bIs32Bit ? ((int *)pIndices)[t * 3 + 0] : ((UINT16 *)pIndices)[t * 3 + 0];
			int i1 = bIs32Bit ? ((int *)pIndices)[t * 3 + 1] : ((UINT16 *)pIndices)[t * 3 + 1];
			int i2 = bIs32Bit ? ((int *)pIndices)[t * 3 + 2] : ((UINT16 *)pIndices)[t * 3 + 2];

			// assuming position is first and a float3
			const SimpleMeshVertex &v0 = pfVerts[i0];
			const SimpleMeshVertex &v1 = pfVerts[i1];
			const SimpleMeshVertex &v2 = pfVerts[i2];

			float dPx0 = v1.m_position.x - v0.m_position.x;
			float dPy0 = v1.m_position.y - v0.m_position.y;
			float dPz0 = v1.m_position.z - v0.m_position.z;

			float dPx1 = v2.m_position.x - v0.m_position.x;
			float dPy1 = v2.m_position.y - v0.m_position.y;
			float dPz1 = v2.m_position.z - v0.m_position.z;

			float nx = dPy0 * dPz1 - dPz0 * dPy1;
			float ny = dPz0 * dPx1 - dPx0 * dPz1;
			float nz = dPx0 * dPy1 - dPy0 * dPx1;

			float fSurfAreaX2 = sqrtf(nx*nx + ny * ny + nz * nz);

			float dTx0 = v1.m_texture.x - v0.m_texture.x;
			float dTy0 = v1.m_texture.y - v0.m_texture.y;

			float dTx1 = v2.m_texture.x - v0.m_texture.x;
			float dTy1 = v2.m_texture.y - v0.m_texture.y;

			float fNormTexAreaX2 = fabsf(dTx0*dTy1 - dTy0 * dTx1);

			if (fNormTexAreaX2 > FLT_EPSILON)
			{
				dAreaRatioSum += ((double)(fSurfAreaX2 / fNormTexAreaX2));
				++iNrContributions;
			}
		}

		float fAverageRatio = iNrContributions > 0 ? ((float)(dAreaRatioSum / iNrContributions)) : 1.0f;
		fMeshSpecificBumpScale = sqrtf(fAverageRatio);
	}

	return fMeshSpecificBumpScale;
}

void SimpeMeshBuilder::SaveSimpleMesh(const SimpleMesh* simpleMesh, const char* filename)
{
	FILE* file;
	fopen_s(&file, filename, "wb");
	fwrite(&simpleMesh->m_vertexCount, 1, sizeof(simpleMesh->m_vertexCount), file);
	fwrite(&simpleMesh->m_vertexStride, 1, sizeof(simpleMesh->m_vertexStride), file);
	fwrite(&simpleMesh->m_vertexAttributeCount, 1, sizeof(simpleMesh->m_vertexAttributeCount), file);
	fwrite(&simpleMesh->m_indexCount, 1, sizeof(simpleMesh->m_indexCount), file);
	fwrite(&simpleMesh->m_indexType, 1, sizeof(simpleMesh->m_indexType), file);
	fwrite(&simpleMesh->m_primitiveType, 1, sizeof(simpleMesh->m_primitiveType), file);
	fwrite(&simpleMesh->m_indexBufferSize, 1, sizeof(simpleMesh->m_indexBufferSize), file);
	fwrite(&simpleMesh->m_vertexBufferSize, 1, sizeof(simpleMesh->m_vertexBufferSize), file);
	fwrite(simpleMesh->m_indexBuffer, simpleMesh->m_indexBufferSize, 1, file);
	fwrite(simpleMesh->m_vertexBuffer, simpleMesh->m_vertexBufferSize, 1, file);
	fclose(file);
}

void SimpeMeshBuilder::LoadSimpleMesh(SimpleMesh* simpleMesh, const char* filename)
{
	FILE* file;
	fopen_s(&file, filename, "rb");
	fread(&simpleMesh->m_vertexCount, 1, sizeof(simpleMesh->m_vertexCount), file);
	fread(&simpleMesh->m_vertexStride, 1, sizeof(simpleMesh->m_vertexStride), file);
	fread(&simpleMesh->m_vertexAttributeCount, 1, sizeof(simpleMesh->m_vertexAttributeCount), file);
	fread(&simpleMesh->m_indexCount, 1, sizeof(simpleMesh->m_indexCount), file);
	fread(&simpleMesh->m_indexType, 1, sizeof(simpleMesh->m_indexType), file);
	fread(&simpleMesh->m_primitiveType, 1, sizeof(simpleMesh->m_primitiveType), file);
	fread(&simpleMesh->m_indexBufferSize, 1, sizeof(simpleMesh->m_indexBufferSize), file);
	fread(&simpleMesh->m_vertexBufferSize, 1, sizeof(simpleMesh->m_vertexBufferSize), file);

	simpleMesh->m_vertexBuffer = new UINT8[simpleMesh->m_vertexBufferSize];
	simpleMesh->m_indexBuffer = new UINT8[simpleMesh->m_indexBufferSize];
	if (simpleMesh->m_indexBuffer)
	{
		fread(simpleMesh->m_indexBuffer, simpleMesh->m_indexBufferSize, 1, file);
	}
	if (simpleMesh->m_vertexBuffer)
	{
		fread(simpleMesh->m_vertexBuffer, simpleMesh->m_vertexBufferSize, 1, file);
	}
	fclose(file);
}

void SimpeMeshBuilder::scaleSimpleMesh(SimpleMesh* simpleMesh, float scale)
{
	SimpleMeshVertex* pvVerts = static_cast<SimpleMeshVertex*>(simpleMesh->m_vertexBuffer);
	const UINT32 iNrVerts = simpleMesh->m_vertexCount;
	for (UINT32 i = 0; i < iNrVerts; ++i)
	{
		pvVerts[i].m_position = XMFLOAT3(pvVerts[i].m_position.x * scale, pvVerts[i].m_position.y * scale, pvVerts[i].m_position.z * scale);
	}
}

