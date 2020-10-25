#include "Camera.h"

Camera::Camera()
{}

Camera::~Camera()
{}

DirectX::XMVECTOR Camera::GetPosition() const
{
    return XMLoadFloat3(&mPosition);
}

DirectX::XMFLOAT3 Camera::GetPosition3f() const
{
    return mPosition;
}

void Camera::SetPosition(float x, float y, float z)
{
    mPosition.x = x;
    mPosition.y = y;
    mPosition.z = z;
}

void Camera::SetPosition(const DirectX::XMFLOAT3 & v)
{
    mPosition = v;
}

DirectX::XMVECTOR Camera::GetRight() const
{
    return XMLoadFloat3(&mRight);
}

DirectX::XMFLOAT3 Camera::GetRight3f() const
{
    return mRight;
}

DirectX::XMVECTOR Camera::GetUp() const
{
    return XMLoadFloat3(&mUp);
}

DirectX::XMFLOAT3 Camera::GetUp3f() const
{
    return mUp;
}

DirectX::XMVECTOR Camera::GetLook() const
{
    return XMLoadFloat3(&mLook);
}

DirectX::XMFLOAT3 Camera::GetLook3f() const
{
    return mLook;
}

float Camera::GetNearZ() const
{
    return mNearZ;
}

float Camera::GetFarZ() const
{
    return mFarZ;
}

float Camera::GetAspect() const
{
    return mAspect;
}

float Camera::getFovY() const
{
    return mFovY;
}

float Camera::getFovX() const
{
    float halfWidth = 0.5f * GetNearWindowWidth();
    return 2.0f * atan(halfWidth / mNearZ);
}

float Camera::GetNearWindowWidth() const
{
    return mAspect * mNearWindowHeight;
}

float Camera::GetNearWindowHeight() const
{
    return mNearWindowHeight;
}

float Camera::GetFarWindowWidth() const
{
    return mAspect * mFarWindowHeight;
}

float Camera::GetFarWindowHeight() const
{
    return mFarWindowHeight;
}

void Camera::SetLens(float fovY, float aspect, float zn, float zf)
{
    mFovY = fovY;
    mAspect = aspect;
    mNearZ = zn;
    mFarZ = zf;

    mNearWindowHeight = 2.0f * mNearZ * tanf(0.5f * mFovY);
    mFarWindowHeight = 2.0f * mFarZ * tanf(0.5f * mFovY);

    DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(mFovY, mAspect, mNearZ, mFarZ);
    DirectX::XMStoreFloat4x4(&mProj, P);
}

void Camera::LookAt(DirectX::XMVECTOR pos, DirectX::XMVECTOR target, DirectX::XMVECTOR worldUp)
{

}

void Camera::LookAt(const DirectX::XMFLOAT3 & pos, const DirectX::XMFLOAT3 & target, const DirectX::XMFLOAT3 & up)
{

}

DirectX::XMMATRIX Camera::GetView() const
{
    return XMLoadFloat4x4(&mView);
}

DirectX::XMMATRIX Camera::GetProj() const
{
    return XMLoadFloat4x4(&mProj);
}

DirectX::XMFLOAT4X4 Camera::GetView4x4f() const
{
    return mView;
}

DirectX::XMFLOAT4X4 Camera::GetProj4x4f() const
{
    return mProj;
}

void Camera::Strafe(float d)
{
    DirectX::XMVECTOR s = DirectX::XMVectorReplicate(d);
    DirectX::XMVECTOR l = DirectX::XMLoadFloat3(&mRight);
    DirectX::XMVECTOR p = DirectX::XMLoadFloat3(&mPosition);
    DirectX::XMStoreFloat3(&mPosition, DirectX::XMVectorMultiplyAdd(s, l, p));
    mViewDirty = true;
}

void Camera::Walk(float d)
{
    DirectX::XMVECTOR s = DirectX::XMVectorReplicate(d);
    DirectX::XMVECTOR l = DirectX::XMLoadFloat3(&mLook);
    DirectX::XMVECTOR p = DirectX::XMLoadFloat3(&mPosition);
    DirectX::XMStoreFloat3(&mPosition, DirectX::XMVectorMultiplyAdd(s, l, p));
    mViewDirty = true;
}

void Camera::Pitch(float angle)
{
    DirectX::XMMATRIX R = DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&mRight), angle);

    DirectX::XMStoreFloat3(&mUp, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&mUp), R));
    DirectX::XMStoreFloat3(&mLook, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&mLook), R));

    mViewDirty = true;
}

void Camera::RotateY(float angle)
{
    DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(angle);

    DirectX::XMStoreFloat3(&mRight, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&mRight), R));
    DirectX::XMStoreFloat3(&mUp, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&mUp), R));
    DirectX::XMStoreFloat3(&mLook, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&mLook), R));

    mViewDirty = true;
}

void Camera::UpdateViewMatrix()
{
    if (mViewDirty)
    {
        DirectX::XMVECTOR R = XMLoadFloat3(&mRight);
        DirectX::XMVECTOR U = XMLoadFloat3(&mUp);
        DirectX::XMVECTOR L = XMLoadFloat3(&mLook);
        DirectX::XMVECTOR P = XMLoadFloat3(&mPosition);

        // Keep camera's axes orthogonal to each other and of unit length.
        L = DirectX::XMVector3Normalize(L);
        U = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(L, R));

        // U, L already ortho-normal, so no need to normalize cross product.
        R = DirectX::XMVector3Cross(U, L);

        // Fill in the view matrix entries.
        float x = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, R));
        float y = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, U));
        float z = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, L));

        XMStoreFloat3(&mRight, R);
        XMStoreFloat3(&mUp, U);
        XMStoreFloat3(&mLook, L);

        mView(0, 0) = mRight.x;
        mView(1, 0) = mRight.y;
        mView(2, 0) = mRight.z;
        mView(3, 0) = x;

        mView(0, 1) = mUp.x;
        mView(1, 1) = mUp.y;
        mView(2, 1) = mUp.z;
        mView(3, 1) = y;

        mView(0, 2) = mLook.x;
        mView(1, 2) = mLook.y;
        mView(2, 2) = mLook.z;
        mView(3, 2) = z;

        mView(0, 3) = 0.0f;
        mView(1, 3) = 0.0f;
        mView(2, 3) = 0.0f;
        mView(3, 3) = 1.0f;

        mViewDirty = false;
    }
}
