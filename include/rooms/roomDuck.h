#pragma once
#include <random>

#include "dxApplication.h"
#include "mesh.h"
#include "environmentMapper.h"
#include "particleSystem.h"

using namespace DirectX;

namespace mini::gk2 {
	class RoomDuck : public DxApplication {
		public:
			using Base = DxApplication;

			explicit RoomDuck (HINSTANCE appInstance);

		protected:
			void Update (const Clock & dt) override;
			void Render () override;

		private:
			static constexpr unsigned int WATER_MAP_WIDTH = 256;
			static constexpr unsigned int WATER_MAP_HEIGHT = 256;

			dx_ptr<ID3D11Buffer> m_cbWorldMatrix;
			dx_ptr<ID3D11Buffer> m_cbViewMatrix;
			dx_ptr<ID3D11Buffer> m_cbProjectionMatrix;
			dx_ptr<ID3D11Buffer> m_cbSurfaceColor;		// pixel shader constant buffer slot 0
			dx_ptr<ID3D11Buffer> m_cbLightPos;			// pixel shader constant buffer slot 1

			Mesh m_meshTeapot;
			Mesh m_waterSurfaceMesh;

			dx_ptr<ID3D11VertexShader> m_phongVS;
			dx_ptr<ID3D11PixelShader> m_phongPS;
			dx_ptr<ID3D11ComputeShader> m_waterCS;
			dx_ptr<ID3D11PixelShader> m_waterPS;
			dx_ptr<ID3D11VertexShader> m_waterVS;

			XMFLOAT4X4 m_projectionMatrix, m_waterSurfaceMatrix;
			XMFLOAT4 m_lightPos[2];

			dx_ptr<ID3D11InputLayout> m_inputlayout;

			dx_ptr<ID3D11Texture2D> m_waterTexture[2];
			dx_ptr<ID3D11ShaderResourceView> m_waterResourceView[2];
			dx_ptr<ID3D11UnorderedAccessView> m_waterUnorderedView[2];
			dx_ptr<ID3D11SamplerState> m_waterSamplerState;

			int m_waterCurrent, m_waterPrev;

			std::random_device m_rd;
			std::mt19937_64 m_gen;
			std::uniform_int_distribution<> m_rain_distr;

		private:
			void m_UpdateCameraCB (DirectX::XMMATRIX viewMtx);
			void m_UpdateCameraCB () { m_UpdateCameraCB (m_camera.getViewMatrix ()); }

			void m_SetWorldMtx (DirectX::XMFLOAT4X4 mtx);
			void m_SetSurfaceColor (DirectX::XMFLOAT4 color);
			void m_SetShaders (const dx_ptr<ID3D11VertexShader> & vs, const dx_ptr<ID3D11PixelShader> & ps);

			void m_DrawMesh (const Mesh & m, DirectX::XMFLOAT4X4 worldMtx);
	};
}