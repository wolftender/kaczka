#include "rooms/roomDuck.h"

using namespace DirectX;

namespace mini::gk2 {
	RoomDuck::RoomDuck (HINSTANCE appInstance) : 
		DxApplication (appInstance, 1366, 768, L"Pokój"),
		m_cbWorldMatrix (m_device.CreateConstantBuffer<XMFLOAT4X4> ()),
		m_cbProjectionMatrix (m_device.CreateConstantBuffer<XMFLOAT4X4> ()),
		m_cbViewMatrix (m_device.CreateConstantBuffer<XMFLOAT4X4, 2> ()),
		m_cbSurfaceColor (m_device.CreateConstantBuffer<XMFLOAT4> ()),
		m_cbLightPos (m_device.CreateConstantBuffer<XMFLOAT4, 2> ()) {

		// projection matrix update
		auto s = m_window.getClientSize ();
		auto ar = static_cast<float>(s.cx) / s.cy;
		XMStoreFloat4x4 (&m_projectionMatrix, XMMatrixPerspectiveFovLH (XM_PIDIV4, ar, 0.01f, 100.0f));
		UpdateBuffer (m_cbProjectionMatrix, m_projectionMatrix);
		m_UpdateCameraCB ();

		XMStoreFloat4x4 (&m_waterSurfaceMatrix, XMMatrixRotationX (XM_PIDIV2) * XMMatrixScaling (10.0f, 10.0f, 10.0f));

		// set light positions
		m_lightPos[0] = { 1.0f, 1.0f, 1.0f, 1.0f };
		m_lightPos[1] = { -1.0f, -1.0f, -1.0f, 1.0f };

		// load meshes
		m_meshTeapot = Mesh::LoadMesh (m_device, L"resources/meshes/teapot.mesh");
		m_waterSurfaceMesh = Mesh::Rectangle (m_device, 1.0f);

		auto vsCode = m_device.LoadByteCode (L"phongVS.cso");
		auto psCode = m_device.LoadByteCode (L"phongPS.cso");
		auto csCode = m_device.LoadByteCode (L"waterCS.cso");

		m_phongVS = m_device.CreateVertexShader (vsCode);
		m_phongPS = m_device.CreatePixelShader (psCode);

		m_waterCS = m_device.CreateComputeShader (csCode);

		vsCode = m_device.LoadByteCode (L"waterVS.cso");
		m_waterVS = m_device.CreateVertexShader (vsCode);

		psCode = m_device.LoadByteCode (L"waterPS.cso");
		m_waterPS = m_device.CreatePixelShader (psCode);

		m_inputlayout = m_device.CreateInputLayout (VertexPositionNormal::Layout, vsCode);

		m_device.context ()->IASetInputLayout (m_inputlayout.get ());
		m_device.context ()->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// We have to make sure all shaders use constant buffers in the same slots!
		// Not all slots will be use by each shader
		ID3D11Buffer * vsb[] = { m_cbWorldMatrix.get (),  m_cbViewMatrix.get (), m_cbProjectionMatrix.get () };
		m_device.context ()->VSSetConstantBuffers (0, 3, vsb); //Vertex Shaders - 0: worldMtx, 1: viewMtx,invViewMtx, 2: projMtx

		ID3D11Buffer * psb[] = { m_cbSurfaceColor.get (), m_cbLightPos.get () };
		m_device.context ()->PSSetConstantBuffers (0, 2, psb); //Pixel Shaders - 0: surfaceColor, 1: lightPos[2]

		// initialize the resource views for compute shader
		Texture2DDescription textureDesc;
		textureDesc.Width = WATER_MAP_WIDTH;
		textureDesc.Height = WATER_MAP_HEIGHT;
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R32_FLOAT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		m_waterTexturePrev = m_device.CreateTexture (textureDesc);

		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

		m_waterTextureCurr = m_device.CreateTexture (textureDesc);

		//////////
		ShaderResourceViewDescription srvDesc;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;

		m_waterPrevView = m_device.CreateShaderResourceView (m_waterTexturePrev, &srvDesc);
		m_waterCurrView = m_device.CreateShaderResourceView (m_waterTextureCurr, &srvDesc);

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		ZeroMemory (&uavDesc, sizeof (uavDesc));
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;

		m_waterCurrUAV = m_device.CreateUnorderedAccessView (m_waterTextureCurr, &uavDesc);

		// sampler states
		SamplerDescription sd;
		sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

		m_waterSamplerState = m_device.CreateSamplerState (sd);
	}

	void RoomDuck::Update (const Clock & c) {
		static ID3D11ShaderResourceView * nullSrv[1] = {nullptr};
		static ID3D11UnorderedAccessView * nullUav[1] = {nullptr};
		
		// camera options
		double dt = c.getFrameTime ();
		HandleCameraInput (dt);

		// update the water texture using compute shader
		auto prevTexSrv = m_waterPrevView.get ();
		auto currTexUav = m_waterCurrUAV.get ();

		m_device.context ()->CSSetShader (m_waterCS.get (), nullptr, 0);
		m_device.context ()->CSSetShaderResources (0, 1, &prevTexSrv);
		m_device.context ()->CSSetUnorderedAccessViews (0, 1, &currTexUav, nullptr);

		// texture is 256x256
		// thread group is 16x16
		// therefore we need 16x16 dispatch
		m_device.context ()->Dispatch (16, 16, 1);

		// this has to be done to flush the buffers
		m_device.context ()->CSSetShader (nullptr, nullptr, 0);
		m_device.context ()->CSSetShaderResources (0, 1, nullSrv);
		m_device.context ()->CSSetUnorderedAccessViews (0, 1, nullUav, nullptr);

		// this is probaby expensive?
		m_device.context ()->CopyResource (m_waterTexturePrev.get (), m_waterTextureCurr.get ());
	}

	void RoomDuck::Render () {
		DxApplication::Render ();

		ResetRenderTarget ();
		m_UpdateCameraCB ();
		m_SetShaders (m_waterVS, m_waterPS);
		UpdateBuffer (m_cbLightPos, m_lightPos);

		// set textures and sampler state for water
		auto waterSrv = m_waterCurrView.get ();
		auto sampler = m_waterSamplerState.get ();

		m_device.context ()->PSSetShaderResources (0, 1, &waterSrv);
		m_device.context ()->PSSetSamplers (0, 1, &sampler);
		
		m_DrawMesh (m_waterSurfaceMesh, m_waterSurfaceMatrix);
	}

	void RoomDuck::m_UpdateCameraCB (DirectX::XMMATRIX viewMtx) {
		XMVECTOR det;
		XMMATRIX invViewMtx = XMMatrixInverse (&det, viewMtx);
		XMFLOAT4X4 view[2];
		XMStoreFloat4x4 (view, viewMtx);
		XMStoreFloat4x4 (view + 1, invViewMtx);
		UpdateBuffer (m_cbViewMatrix, view);
	}

	void RoomDuck::m_SetWorldMtx (DirectX::XMFLOAT4X4 mtx) {
		UpdateBuffer (m_cbWorldMatrix, mtx);
	}

	void RoomDuck::m_SetSurfaceColor (DirectX::XMFLOAT4 color) {
		UpdateBuffer (m_cbSurfaceColor, color);
	}

	void RoomDuck::m_SetShaders (const dx_ptr<ID3D11VertexShader> & vs, const dx_ptr<ID3D11PixelShader> & ps) {
		m_device.context ()->VSSetShader (vs.get (), nullptr, 0);
		m_device.context ()->PSSetShader (ps.get (), nullptr, 0);
	}

	void RoomDuck::m_DrawMesh (const Mesh & m, DirectX::XMFLOAT4X4 worldMtx) {
		m_SetWorldMtx (worldMtx);
		m.Render (m_device.context ());
	}
}