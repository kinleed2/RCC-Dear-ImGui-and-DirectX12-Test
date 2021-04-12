//***************************************************************************************
// Camera.h by Frank Luna (C) 2011 All Rights Reserved.
//   
// Simple first person style camera class that lets the viewer explore the 3D scene.
//   -It keeps track of the camera coordinate system relative to the world space
//    so that the view matrix can be constructed.  
//   -It keeps track of the viewing frustum of the camera so that the projection
//    matrix can be obtained.
//***************************************************************************************

#ifndef CAMERA_H
#define CAMERA_H

#include "../DirectXTK12/Inc/SimpleMath.h"
#include <DirectXMath.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

class Camera
{
public:

	Camera();
	~Camera();

	// Get/Set world camera position.
	Vector3 GetPosition()const;
	void SetPosition(const Vector3& v);
	
	// Get camera basis vectors.
	Vector3 GetRight()const;
	Vector3 GetUp()const;
	Vector3 GetLook()const;

	// Get frustum properties.
	float GetNearZ()const;
	float GetFarZ()const;
	float GetAspect()const;
	float GetFovY()const;
	float GetFovX()const;

	// Get near and far plane dimensions in view space coordinates.
	float GetNearWindowWidth()const;
	float GetNearWindowHeight()const;
	float GetFarWindowWidth()const;
	float GetFarWindowHeight()const;
	
	// Set frustum.
	void SetLens(float fovY, float aspect, float zn, float zf);

	// Define camera space via LookAt parameters.
	void LookAtFXMVECTOR(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp);
	void LookAt(const Vector3& pos, const Vector3& target, const Vector3& up);

	// Get View/Proj matrices.
	Matrix GetView()const;
	Matrix GetProj()const;

	// Strafe/Walk the camera a distance d.
	void Strafe(float d);
	void Walk(float d);

	// Rotate the camera.
	void Pitch(float angle);
	void RotateY(float angle);

	// After modifying camera position/orientation, call to rebuild the view matrix.
	void UpdateViewMatrix();

private:

	// Camera coordinate system with coordinates relative to world space.
	Vector3 mPosition = { 0.0f, 0.0f, 0.0f };
	Vector3 mRight = { 1.0f, 0.0f, 0.0f };
	Vector3 mUp = { 0.0f, 1.0f, 0.0f };
	Vector3 mLook = { 0.0f, 0.0f, 1.0f };

	// Cache frustum properties.
	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	bool mViewDirty = true;

	// Cache View/Proj matrices.
	Matrix mView = Matrix::Identity;
	Matrix mProj = Matrix::Identity;

};

#endif // CAMERA_H