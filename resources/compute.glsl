#version 430 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

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

readonly restrict layout(std430, binding = 0) buffer u_readBodies
{
	Object readBodies[];
};

//bodies is basically write bodies
restrict layout(std430, binding = 1) buffer u_writeBodies
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

vec3 boxDimensions = vec3(20, 20, 20);

void main() 
{

	uint i = gl_WorkGroupID.x;
	bodies[i] = readBodies[i];

	//gravity
	bodies[i].acceleration = vec3(0, -9.81, 0);

	//drag
	applyDrag(bodies[i]);


	//colisions


	
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