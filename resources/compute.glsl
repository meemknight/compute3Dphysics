#version 430 core

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

uniform int u_size;
uniform float u_deltaTime;

const int TYPE_CIRCLE = 0;
const int TYPE_BOX = 1;
const int TYPE_CILINDRU = 2;

const float MAX_VELOCITY = 10000;
const float MAX_ACCELERATION = 10000;
const float MAX_AIR_DRAG = 100;

struct Object
{
	vec3 position;       // offset: 0
	float padding1;      // vec3 needs vec4 alignment

	vec3 shape;          // offset: 16
	float padding2;

	vec3 velocity;       // offset: 32
	float padding3;

	vec3 acceleration;   // offset: 48
	float padding4;

	float mass;          // offset: 64
	float bouncyness;    // offset: 68
	int type;            // offset: 72
	float staticFriction;// offset: 76

	float dynamicFriction; // offset: 80
	float padding5[3];   // ensure 16-byte alignment
};

readonly restrict layout(std430, binding = 0) coherent buffer u_readBodies
{
	Object readBodies[];
};

//bodies is basically write bodies
restrict layout(std430, binding = 1) coherent buffer u_writeBodies
{
	Object bodies[];
};

vec3 getMin(inout Object b)
{
	if (b.type == TYPE_CIRCLE)
	{
		vec3 rez = b.position;
		rez -= vec3(b.shape.x, b.shape.x, b.shape.x);
		return rez;
	}
	else if (b.type == TYPE_BOX)
	{
		vec3 rez = b.position;
		rez -= vec3(b.shape) / 2.f;
		return rez;
	}
	else if (b.type == TYPE_CILINDRU)
	{
		vec3 rez = b.position;
		rez -= vec3(b.shape.x, b.shape.y / 2.f, b.shape.x);
		return rez;
	}
}

vec3 getMax(inout Object b)
{
	if (b.type == TYPE_CIRCLE)
	{
		vec3 rez = b.position;
		rez += vec3(b.shape.x, b.shape.x, b.shape.x);
		return rez;
	}
	else if (b.type == TYPE_BOX)
	{
		vec3 rez = b.position;
		rez += vec3(b.shape) / 2.f;
		return rez;
	}if (b.type == TYPE_CILINDRU)
	{
		vec3 rez = b.position;
		rez += vec3(b.shape.x, b.shape.y / 2.f, b.shape.x);
		return rez;
	}
}

void updateForces(inout Object object)
{

	//object.velocity += object.acceleration * vec3(u_deltaTime);
	//object.position += object.velocity * vec3(u_deltaTime);

	object.acceleration = clamp(object.acceleration,
		vec3(-MAX_ACCELERATION), vec3(MAX_ACCELERATION));
	
	//Symplectic Euler
	object.velocity += object.acceleration * u_deltaTime * 0.5f;
	object.velocity = clamp(object.velocity, vec3(-MAX_VELOCITY), vec3(MAX_VELOCITY));
	
	object.position += object.velocity * u_deltaTime;
	
	object.velocity += object.acceleration * u_deltaTime * 0.5f;
	object.velocity = clamp(object.velocity, vec3(-MAX_VELOCITY), vec3(MAX_VELOCITY));
	
	if (abs(object.velocity.x) < 0.00001) { object.velocity.x = 0; }
	if (abs(object.velocity.y) < 0.00001) { object.velocity.y = 0; }
	
	object.acceleration = vec3(0,0,0);
}

void applyDrag(inout Object object)
{
	vec3 dragForce = 0.1f * -object.velocity * abs(object.velocity) / 2.f;
	float length = length(dragForce);
	if (length != 0)
	{
		if (length > MAX_AIR_DRAG)
		{
			dragForce /= length;
			dragForce *= MAX_AIR_DRAG;
		}

		object.acceleration += dragForce;
	}
}

void normalizeSafe(inout vec3 v)
{
	float l = length(v);

	if (l <= 0.00000001)
	{
		v = vec3(1,0,0);
	}
	else
	{
		v /= l;
	}
}

bool AABBvsSphere(inout Object abox, int indexB, out float penetration, out vec3 normal)
{

	Object bsphere = readBodies[indexB];

	// Vector from A to B
	vec3 n = bsphere.position - abox.position;

	// Closest point on A to center of B
	vec3 closest = n;

	vec3 aboxMax = getMax(abox);
	vec3 aboxMin = getMin(abox);

	// Calculate half extents along each axis for the AABB
	float x_extent = (aboxMax.x - aboxMin.x) / 2.0f;
	float y_extent = (aboxMax.y - aboxMin.y) / 2.0f;
	float z_extent = (aboxMax.z - aboxMin.z) / 2.0f;

	// Clamp point to edges of the AABB
	closest.x = clamp(closest.x, -x_extent, x_extent);
	closest.y = clamp(closest.y, -y_extent, y_extent);
	closest.z = clamp(closest.z, -z_extent, z_extent);

	bool inside = false;

	// Check if sphere center is inside the AABB
	if (n == closest)
	{
		inside = true;

		// Clamp to closest extent along the axis with the largest component
		if (abs(n.x) > abs(n.y) && abs(n.x) > abs(n.z))
		{
			closest.x = (closest.x > 0) ? x_extent : -x_extent;
		}
		else if (abs(n.y) > abs(n.z))
		{
			closest.y = (closest.y > 0) ? y_extent : -y_extent;
		}
		else
		{
			closest.z = (closest.z > 0) ? z_extent : -z_extent;
		}
	}

	// Calculate the vector from the closest point on A to the center of B
	vec3 normal3D = n - closest;
	float d = dot(normal3D, normal3D);
	float r = bsphere.shape.r;

	// Early out if the distance to the closest point is greater than the sphere's radius and the sphere is not inside the AABB
	if (d > r * r && !inside)
	{
		return false;
	}

	// Calculate the actual distance if needed
	d = sqrt(d);

	// Set the collision normal and penetration depth
	if (inside)
	{
		normal = -normalize(normal3D);
		penetration = r - d;
	}
	else
	{
		normal = normalize(normal3D);
		penetration = r - d;
	}

	return true;
}


bool CylindervsCylinder(inout Object a, int indexB, out float penetration, out vec3 normal)
{

	Object b = readBodies[indexB];

	// Vector from A to B
	vec3 n = b.position - a.position;

	vec3 aMax = getMax(a);
	vec3 aMin = getMin(a);
	vec3 bMax = getMax(b);
	vec3 bMin = getMin(b);

	// Calculate half extents along each axis for each object
	float a_extent_x = (aMax.x - aMin.x) / 2.0f;
	float b_extent_x = (bMax.x - bMin.x) / 2.0f;
	float a_extent_y = (aMax.y - aMin.y) / 2.0f;
	float b_extent_y = (bMax.y - bMin.y) / 2.0f;
	float a_extent_z = (aMax.z - aMin.z) / 2.0f;
	float b_extent_z = (bMax.z - bMin.z) / 2.0f;

	// Calculate overlaps on each axis
	float x_overlap = a_extent_x + b_extent_x - abs(n.x);
	float y_overlap = a_extent_y + b_extent_y - abs(n.y);
	float z_overlap = a_extent_z + b_extent_z - abs(n.z);

	vec2 distantaXZ = vec2(n.x, n.z);

	float r = a.shape.r + b.shape.r;
	float rSquared = r * r;
	float distanceSquared = ((a.position.x - b.position.x) * (a.position.x - b.position.x)
		+ (a.position.z - b.position.z) * (a.position.z - b.position.z));
	bool overlapXZ = rSquared > distanceSquared;


	// SAT test on x, y, and z axes
	if (y_overlap > 0 && overlapXZ)
	{
		float XZdist = sqrt(distanceSquared);
		float xzOverlap = r - XZdist;

		// Determine the axis of least penetration
		if (y_overlap < xzOverlap)
		{
			normal = (n.y < 0) ? vec3(0, -1, 0) : vec3(0, 1, 0);
			penetration = y_overlap;
		}
		else
		{

			if (distantaXZ.x == 0 && distantaXZ.y == 0)
			{
				normal = vec3(-1, 0, 0);
				penetration = r;
			}
			else
			{


				distantaXZ /= XZdist; //normalize
				normal.x = distantaXZ.x;
				normal.y = 0;
				normal.z = distantaXZ.y;

				penetration = xzOverlap;
			}
		}

		return true;
	}

	return false;

}


//a is cube
//b is cylinder
bool AABBvsCylinder(inout Object a, int indexB, out float penetration, out vec3 normal)
{
	Object b = readBodies[indexB];

	// Vector from A to B
	vec3 n = b.position - a.position;

	vec3 aMax = getMax(a);
	vec3 aMin = getMin(a);
	vec3 bMax = getMax(b);
	vec3 bMin = getMin(b);

	// Calculate half extents along each axis for each object
	float a_extent_x = (aMax.x - aMin.x) / 2.0f;
	float b_extent_x = (bMax.x - bMin.x) / 2.0f;
	float a_extent_y = (aMax.y - aMin.y) / 2.0f;
	float b_extent_y = (bMax.y - bMin.y) / 2.0f;
	float a_extent_z = (aMax.z - aMin.z) / 2.0f;
	float b_extent_z = (bMax.z - bMin.z) / 2.0f;

	// Calculate overlaps on each axis
	float x_overlap = a_extent_x + b_extent_x - abs(n.x);
	float y_overlap = a_extent_y + b_extent_y - abs(n.y);
	float z_overlap = a_extent_z + b_extent_z - abs(n.z);


	// Closest point on A to center of B
	vec3 closest = n;

	// Calculate half extents along each axis for the AABB
	float x_extent = (aMax.x - aMin.x) / 2.0f;
	float y_extent = (aMax.y - aMin.y) / 2.0f;
	float z_extent = (aMax.z - aMin.z) / 2.0f;

	// Clamp point to edges of the AABB
	closest.x = clamp(closest.x, -x_extent, x_extent);
	closest.y = clamp(closest.y, -y_extent, y_extent);
	closest.z = clamp(closest.z, -z_extent, z_extent);

	bool inside = false;

	// Check if sphere center is inside the AABB
	if (n == closest)
	{
		inside = true;

		// Clamp to closest extent along the axis with the largest component
		if (abs(n.x) > abs(n.y) && abs(n.x) > abs(n.z))
		{
			closest.x = (closest.x > 0) ? x_extent : -x_extent;
		}
		else if (abs(n.y) > abs(n.z))
		{
			closest.y = (closest.y > 0) ? y_extent : -y_extent;
		}
		else
		{
			closest.z = (closest.z > 0) ? z_extent : -z_extent;
		}
	}

	vec3 closestWorldPos = closest + a.position;


	float distantaClosestPoint = distance(vec3(closestWorldPos.x, 0.f, closestWorldPos.z), vec3(b.position.x, 0.f, b.position.z));
	float overlapXZ = b.shape.x - distantaClosestPoint;

	// SAT test on x, y, and z axes
	if (inside || (y_overlap > 0 && overlapXZ > 0))
	{

		// Determine the axis of least penetration
		if (y_overlap < overlapXZ)
		{
			normal = (n.y < 0) ? vec3(0, -1, 0) : vec3(0, 1, 0);
			penetration = y_overlap;
		}
		else
		{
			//cerc patrat

			// Calculate the vector from the closest point on A to the center of B
			vec3 normal3D = n - closest; normal3D.y = 0;
			float d = dot(normal3D, normal3D);
			float r = b.shape.r;

			if (normal3D.x == 0 && normal3D.z == 0)
			{
				normal = -vec3(1,0,0);
				penetration = r;
				return true;
			}

			d = sqrt(d);

			// Set the collision normal and penetration depth
			if (inside)
			{
				normal = -normalize(normal3D);
				penetration = r - d;
			}
			else
			{
				normal = normalize(normal3D);
				penetration = r - d;
			}
		}

		return true;
	}

	return false;

}

bool AABBvsAABB(inout Object a, int indexB , out float penetration, out vec3 normal)
{

	Object b = readBodies[indexB];

	// Vector from A to B
	vec3 n = b.position - a.position;

	vec3 aMax = getMax(a);
	vec3 aMin = getMin(a);
	vec3 bMax = getMax(b);
	vec3 bMin = getMin(b);

	// Calculate half extents along each axis for each object
	float a_extent_x = (aMax.x - aMin.x) / 2.0f;
	float b_extent_x = (bMax.x - bMin.x) / 2.0f;
	float a_extent_y = (aMax.y - aMin.y) / 2.0f;
	float b_extent_y = (bMax.y - bMin.y) / 2.0f;
	float a_extent_z = (aMax.z - aMin.z) / 2.0f;
	float b_extent_z = (bMax.z - bMin.z) / 2.0f;

	// Calculate overlaps on each axis
	float x_overlap = a_extent_x + b_extent_x - abs(n.x);
	float y_overlap = a_extent_y + b_extent_y - abs(n.y);
	float z_overlap = a_extent_z + b_extent_z - abs(n.z);

	// SAT test on x, y, and z axes
	if (x_overlap > 0 && y_overlap > 0 && z_overlap > 0)
	{
		// Determine the axis of least penetration
		if (x_overlap < y_overlap && x_overlap < z_overlap)
		{
			normal = (n.x < 0) ? vec3(-1, 0, 0) : vec3(1, 0, 0);
			penetration = x_overlap;
		}
		else if (y_overlap < z_overlap)
		{
			normal = (n.y < 0) ? vec3(0, -1, 0) : vec3(0, 1, 0);
			penetration = y_overlap;
		}
		else
		{
			normal = (n.z < 0) ? vec3(0, 0, -1) : vec3(0, 0, 1);
			penetration = z_overlap;
		}
		return true;
	}

	return false;
}

bool CirclevsCircle(inout Object a, int indexB, out float penetration, out vec3 normal)
{

	Object b = readBodies[indexB];

	float r = a.shape.r + b.shape.r;
	float rSquared = r * r;
	float distanceSquared = ((a.position.x - b.position.x) * (a.position.x - b.position.x)
		+ (a.position.y - b.position.y) * (a.position.y - b.position.y)
		+ (a.position.z - b.position.z) * (a.position.z - b.position.z));

	bool rez = rSquared > distanceSquared;

	if (rez)
	{
		normal = b.position - a.position;
		normalizeSafe(normal);
		penetration = r - sqrt(distanceSquared);
	}

	return rez;
}


//a is sphere
//b is cylinder
bool SpherevsCylinder(inout Object a, int indexB, out float penetration, out vec3 normal)
{
	Object b = readBodies[indexB];

	// Vector from A to B
	vec3 n = b.position - a.position;

	vec3 aMax = getMax(a);
	vec3 aMin = getMin(a);
	vec3 bMax = getMax(b);
	vec3 bMin = getMin(b);

	// Calculate half extents along each axis for each object
	float a_extent_x = (aMax.x - aMin.x) / 2.0f;
	float b_extent_x = (bMax.x - bMin.x) / 2.0f;
	float a_extent_y = (aMax.y - aMin.y) / 2.0f;
	float b_extent_y = (bMax.y - bMin.y) / 2.0f;
	float a_extent_z = (aMax.z - aMin.z) / 2.0f;
	float b_extent_z = (bMax.z - bMin.z) / 2.0f;

	// Calculate overlaps on each axis
	//float x_overlap = a_extent_x + b_extent_x - abs(n.x);
	//float y_overlap = a_extent_y + b_extent_y - abs(n.y);
	//float z_overlap = a_extent_z + b_extent_z - abs(n.z);

	float y_overlap = 0;
	bool inside = false;

	// Closest point on B to center of A sphere
	vec3 closest = -n;
	{

		// Calculate half extents along each axis for the AABB
		float x_extent = (bMax.x - bMin.x) / 2.0f;
		float y_extent = (bMax.y - bMin.y) / 2.0f;
		float z_extent = (bMax.z - bMin.z) / 2.0f;

		// Clamp point to edges of the AABB
		closest.x = clamp(closest.x, -x_extent, x_extent);
		closest.y = clamp(closest.y, -y_extent, y_extent);
		closest.z = clamp(closest.z, -z_extent, z_extent);


		// Check if sphere center is inside the AABB
		if (n == closest)
		{
			inside = true;

			// Clamp to closest extent along the axis with the largest component
			if (abs(n.x) > abs(n.y) && abs(n.x) > abs(n.z))
			{
				closest.x = (closest.x > 0) ? x_extent : -x_extent;
			}
			else if (abs(n.y) > abs(n.z))
			{
				closest.y = (closest.y > 0) ? y_extent : -y_extent;
			}
			else
			{
				closest.z = (closest.z > 0) ? z_extent : -z_extent;
			}
		}

		vec3 closestWorldPos = closest + b.position;

		y_overlap = a.shape.x - abs(a.position.y - closestWorldPos.y);
	}

	float overlapXZ = 0;
	float r = a.shape.r + b.shape.r;
	float distanceSquared = 0;
	{
		vec2 distantaXZ = vec2(n.x, n.z);
		float rSquared = r * r;
		float distanceSquared = ((a.position.x - b.position.x) * (a.position.x - b.position.x)
			+ (a.position.z - b.position.z) * (a.position.z - b.position.z));
		float XZdist = sqrt(distanceSquared);
		overlapXZ = r - XZdist;
	}


	// SAT test on x, y, and z axes
	if (inside || (y_overlap > 0 && overlapXZ > 0))
	{

		// Determine the axis of least penetration
		if (y_overlap < overlapXZ)
		{
			normal = (n.y < 0) ? vec3(0, -1, 0) : vec3(0, 1, 0);
			penetration = y_overlap;

			//cerc patrat

			//// Calculate the vector from the closest point on A to the center of B
			//vec3 normal3D = a.position - (b.position + closest);
			//float d = dot(normal3D, normal3D);
			//float r = a.shape.r;
			//
			//if (normal3D.x == 0 && normal3D.z == 0)
			//{
			//	normal = -vec3({1,0,0});
			//	penetration = r;
			//	return true;
			//}
			//
			//d = sqrt(d);
			//
			//// Set the collision normal and penetration depth
			//if (inside)
			//{
			//	normal = -normalize(normal3D);
			//	penetration = r - d;
			//}
			//else
			//{
			//	normal = normalize(normal3D);
			//	penetration = r - d;
			//}
		}
		else
		{
			//cerc cerc pe XZ
			normal = b.position - a.position;
			normal.y = 0;
			normalizeSafe(normal);
			penetration = overlapXZ;

		}

		return true;
	}

	return false;

}

void positionalCorrection(inout Object A, Object B, vec3 n,
	float penetrationDepth, float aInverseMass, float bInverseMass)
{
	const float percent = 0.2; // usually 20% to 80%
	const float slop = 0.01; // usually 0.01 to 0.1 

	vec3 correction = (max(penetrationDepth - slop, 0.0f) / (aInverseMass + bInverseMass)) * percent * n;

	A.position -= aInverseMass * correction;
	//B.position += bInverseMass * correction;
};


float pythagoreanSolve(float fA, float fB)
{
	return sqrt(fA * fA + fB * fB);
}

void applyFriction(inout Object A, Object B, vec3 tangent, vec3 rv,
	float aInverseMass, float bInverseMass, float j)
{
	// Solve for magnitude to apply along the friction vector
	float jt = -dot(rv, tangent);
	jt = jt / (aInverseMass + bInverseMass);

	// PythagoreanSolve = A^2 + B^2 = C^2, solving for C given A and B
	// Use to approximate mu given friction coefficients of each body
	float mu = pythagoreanSolve(A.staticFriction, B.staticFriction);

	// Clamp magnitude of friction and create impulse vector
	//(Coulomb's Law) Ff<=Fn
	vec3 frictionImpulse = vec3(0,0,0);
	if (abs(jt) < j * mu)
	{
		frictionImpulse = jt * tangent;
	}
	else
	{
		float dynamicFriction = pythagoreanSolve(A.dynamicFriction, B.dynamicFriction);
		frictionImpulse = -j * tangent * dynamicFriction;
	}

	// Apply
	A.velocity -= (aInverseMass)*frictionImpulse;
	//B.velocity += (bInverseMass)*frictionImpulse;

};

//
const float INFINITY = uintBitsToFloat(0x7F800000);

void impulseResolution(inout Object A, int indexB, vec3 normal,
	float velAlongNormal, float penetrationDepth)
{
	Object B = readBodies[indexB];

	//calculate elasticity
	float e = min(A.bouncyness, B.bouncyness);
	//float e = 0.9;

	float massInverseA = 1.f / A.mass;
	float massInverseB = 1.f / B.mass;

	if (A.mass == 0 || A.mass == INFINITY) { massInverseA = 0; }
	if (B.mass == 0 || B.mass == INFINITY) { massInverseB = 0; }

	// Calculate impulse scalar
	float j = -(1.f + e) * velAlongNormal;
	j /= massInverseA + massInverseB;

	// Apply impulse
	vec3 impulse = j * normal;
	A.velocity -= massInverseA * impulse;
	//B.velocity += massInverseB * impulse;

	positionalCorrection(A, B, normal, penetrationDepth, massInverseA, massInverseB);

	{

		// Re-calculate relative velocity after normal impulse
		// is applied (impulse from first article, this code comes
		// directly thereafter in the same resolve function)

		vec3 rv = B.velocity - A.velocity;

		// Solve for the tangent vector
		vec3 tangent = rv - dot(rv, normal) * normal;

		normalizeSafe(tangent);

		applyFriction(A, B, tangent, rv, massInverseA, massInverseB, j);
	}
};



vec3 boxDimensions = vec3(20, 20, 20);

void main() 
{

	uint i = gl_WorkGroupID.x * gl_WorkGroupSize.x + gl_LocalInvocationID.x;
	bodies[i] = readBodies[i];

	//gravity
	bodies[i].acceleration += vec3(0, -9.81, 0);

	//drag
	applyDrag(bodies[i]);

	//colisions
	for (int j = 0; j < u_size; j++)
	{

		if (i == j) { continue; }

		//auto &A = bodies[i];
		//auto &B = readBodies[j];

		if (bodies[i].type == TYPE_CIRCLE &&
			readBodies[j].type == TYPE_CIRCLE
			)
		{
			vec3 normal = vec3(0,0,0);
			float penetration = 0;

			if (CirclevsCircle(
				bodies[i], j,
				penetration, normal))
			{
				vec3 relativeVelocity = readBodies[j].velocity - bodies[i].velocity;
				float velAlongNormal = dot(relativeVelocity, normal);

				// Do not resolve if velocities are separating
				if (velAlongNormal > 0)
				{

				}
				else
				{
					impulseResolution(bodies[i], j, normal, velAlongNormal, penetration);
				}
			}

		}
		else
			if (bodies[i].type == TYPE_BOX &&
				readBodies[j].type == TYPE_BOX
				)
			{
				vec3 normal = vec3(0);
				float penetration = 0;

				if (AABBvsAABB(
					bodies[i], j,
					penetration, normal))
				{
					vec3 relativeVelocity = readBodies[j].velocity - bodies[i].velocity;
					float velAlongNormal = dot(relativeVelocity, normal);

					// Do not resolve if velocities are separating
					if (velAlongNormal > 0)
					{

					}
					else
					{
						impulseResolution(bodies[i], j, normal, velAlongNormal, penetration);
					}
				}

			}
			else
				if (bodies[i].type == TYPE_BOX &&
					readBodies[j].type == TYPE_CIRCLE
					)
				{

					vec3 normal = vec3(0,0,0);
					float penetration = 0;

					if (AABBvsSphere(
						bodies[i], j,
						penetration, normal))
					{
						vec3 relativeVelocity = readBodies[j].velocity - bodies[i].velocity;
						float velAlongNormal = dot(relativeVelocity, normal);

						// Do not resolve if velocities are separating
						if (velAlongNormal > 0)
						{

						}
						else
						{
							impulseResolution(bodies[i], j, normal, velAlongNormal, penetration);
						}
					}
				}
				else
					if (bodies[i].type == TYPE_CILINDRU &&
						readBodies[j].type == TYPE_CILINDRU
						)
					{

						vec3 normal = vec3(0,0,0);
						float penetration = 0;

						if (CylindervsCylinder(
							bodies[i], j,
							penetration, normal))
						{
							vec3 relativeVelocity = readBodies[j].velocity - bodies[i].velocity;
							float velAlongNormal = dot(relativeVelocity, normal);

							// Do not resolve if velocities are separating
							if (velAlongNormal > 0)
							{

							}
							else
							{
								impulseResolution(bodies[i], j, normal, velAlongNormal, penetration);
							}
						}
					}
					else if (bodies[i].type == TYPE_BOX && readBodies[j].type == TYPE_CILINDRU)
					{
						vec3 normal = vec3(0,0,0);
						float penetration = 0;

						if (AABBvsCylinder(
							bodies[i], j,
							penetration, normal))
						{
							vec3 relativeVelocity = readBodies[j].velocity - bodies[i].velocity;
							float velAlongNormal = dot(relativeVelocity, normal);

							// Do not resolve if velocities are separating
							if (velAlongNormal > 0)
							{

							}
							else
							{
								impulseResolution(bodies[i], j, normal, velAlongNormal, penetration);
							}
						}
					}
					else if (bodies[i].type == TYPE_CIRCLE && readBodies[j].type == TYPE_CILINDRU)
					{
						vec3 normal = vec3(0, 0, 0);
						float penetration = 0;

						if (SpherevsCylinder(
							bodies[i], j,
							penetration, normal))
						{
							vec3 relativeVelocity = readBodies[j].velocity - bodies[i].velocity;
							float velAlongNormal = dot(relativeVelocity, normal);

							// Do not resolve if velocities are separating
							if (velAlongNormal > 0)
							{

							}
							else
							{
								impulseResolution(bodies[i], j, normal, velAlongNormal, penetration);
							}
						}
					}



	}
	
	//compute euler integration
	updateForces(bodies[i]);
	

	//hit floor and edges!!!
	vec3 minPos = getMin(bodies[i]);
	vec3 maxPos = getMax(bodies[i]);

	float boxDown = -boxDimensions.y / 2;

	if (minPos.y < boxDown)
	{
		float extra = boxDown - minPos.y;
		bodies[i].position.y += extra;
		bodies[i].velocity.y *= -1;
	}

	if (maxPos.y > boxDimensions.y / 2)
	{
		float extra = maxPos.y - boxDimensions.y / 2;
		bodies[i].position.y -= extra;
		bodies[i].velocity.y *= -1;
	}

	if (minPos.x < -boxDimensions.x / 2)
	{
		float extra = -boxDimensions.x / 2 - minPos.x;
		bodies[i].position.x += extra;
		bodies[i].velocity.x *= -1;
	}

	if (minPos.z < -boxDimensions.z / 2)
	{
		float extra = -boxDimensions.z / 2 - minPos.z;
		bodies[i].position.z += extra;
		bodies[i].velocity.z *= -1;
	}

	if (maxPos.x > boxDimensions.x / 2)
	{
		float extra = maxPos.x - boxDimensions.x / 2;
		bodies[i].position.x -= extra;
		bodies[i].velocity.x *= -1;
	}

	if (maxPos.z > boxDimensions.z / 2)
	{
		float extra = maxPos.z - boxDimensions.z / 2;
		bodies[i].position.z -= extra;
		bodies[i].velocity.z *= -1;
	}

}