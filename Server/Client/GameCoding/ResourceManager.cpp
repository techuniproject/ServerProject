#include "pch.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "Sprite.h"
#include "Flipbook.h"
#include "Tilemap.h"
#include "Sound.h"

ResourceManager::~ResourceManager()
{
	Clear();// 맵만 비우면 shared_ptr 해제. 외부가 잡고 있으면 유지됨.
}

void ResourceManager::Init(HWND hwnd, fs::path resourcePath)
{
	_hwnd = hwnd;
	_resourcePath = std::move(resourcePath);

	//fs::current_path();
	//_resourcePath.relative_path();
	//fs::absolute(_resourcePath);
}

void ResourceManager::Clear()
{
	// 외부가 held 중이면 살아있고, 아니면 파괴됨.
	_sounds.clear();
	_tilemaps.clear();
	_flipbooks.clear();
	_sprites.clear();
	_textures.clear();
}

shared_ptr<Texture> ResourceManager::GetTexture(const wstring& key) const
{
	if (auto it = _textures.find(key); it != _textures.end())
		return it->second;
	return nullptr;
}

shared_ptr<Texture> ResourceManager::LoadTexture(const wstring& key, const wstring& path, uint32 transparent /*= RGB(255, 0, 255)*/)
{
	/*if (_textures.find(key) != _textures.end())
		return _textures[key];*/
	if (auto it = _textures.find(key); it != _textures.end())
		return it->second;

	fs::path fullPath = _resourcePath / path;

	shared_ptr<Texture> tex = std::make_shared<Texture>();
	tex->LoadBmp(_hwnd, fullPath.c_str());
	tex->SetTransparent(transparent);
	_textures[key] = tex;//key가없으면 기본생성 후 대입, key있으면 바꿔치기
	//_textures.emplace(key, tex);//인자 전달후 직접 생성, key 있으면 삽입 실패

	return tex;
}

shared_ptr<Sprite> ResourceManager::GetSprite(const wstring& key) const
{
	if (auto it = _sprites.find(key); it != _sprites.end())
		return it->second;
	return nullptr;
}

shared_ptr<Sprite> ResourceManager::CreateSprite(const wstring& key, shared_ptr<Texture> texture, int32 x, int32 y, int32 cx, int32 cy)
{
	/*if (_sprites.find(key) != _sprites.end())
		return _sprites[key];*/

	if (auto it = _sprites.find(key); it != _sprites.end())
		return it->second;

	if (cx == 0)
		cx = texture->GetSize().x;

	if (cy == 0)
		cy = texture->GetSize().y;

	shared_ptr<Sprite> sp = std::make_shared<Sprite>(texture, x, y, cx, cy);
	_sprites[key] = sp;
	//_sprites.emplace(key, sp);

	return sp;
}

shared_ptr<Flipbook> ResourceManager::GetFlipbook(const std::wstring& key) const
{
	if (auto it = _flipbooks.find(key); it != _flipbooks.end())
		return it->second;
	return nullptr;
}

shared_ptr<Flipbook> ResourceManager::CreateFlipbook(const wstring& key)
{
	if (auto it = _flipbooks.find(key); it != _flipbooks.end())
		return it->second;

	shared_ptr<Flipbook> fb = std::make_shared<Flipbook>();
	//_flipbooks.emplace(key, fb);
	_flipbooks[key] = fb;

	return fb;
}

shared_ptr<Tilemap> ResourceManager::GetTilemap(const std::wstring& key) const
{
	if (auto it = _tilemaps.find(key); it != _tilemaps.end())
		return it->second;
	return nullptr;
}


shared_ptr<Tilemap> ResourceManager::CreateTilemap(const wstring& key)
{
	if (auto it = _tilemaps.find(key); it != _tilemaps.end())
		return it->second;

	shared_ptr<Tilemap> tm = std::make_shared<Tilemap>();
	//_tilemaps.emplace(key, tm);
	_tilemaps[key] = tm;

	return tm;
}

void ResourceManager::SaveTilemap(const wstring& key, const wstring& path)
{
	shared_ptr<Tilemap> tilemap = GetTilemap(key);
	if (!tilemap) return;

	fs::path fullPath = _resourcePath / path;
	tilemap->SaveFile(fullPath);
}

shared_ptr<Tilemap>  ResourceManager::LoadTilemap(const wstring& key, const wstring& path)
{
	if (auto it = _tilemaps.find(key); it != _tilemaps.end())
	{
		fs::path fullPath = _resourcePath / path;
		it->second->LoadFile(fullPath);
		return it->second;
	}

	//shared_ptr<Tilemap> tm = std::make_shared<Tilemap>();
	//tm->LoadFile(fullPath);

	//_tilemaps.emplace(key, tm);
	return nullptr;
}

shared_ptr<Sound> ResourceManager::GetSound(const std::wstring& key) const
{
	if (auto it = _sounds.find(key); it != _sounds.end())
		return it->second;
	return nullptr;
}

shared_ptr<Sound> ResourceManager::LoadSound(const wstring& key, const wstring& path)
{
	if (auto it = _sounds.find(key); it != _sounds.end())
		return it->second;

	fs::path fullPath = _resourcePath / path;

	shared_ptr<Sound> snd = std::make_shared<Sound>();
	snd->LoadWave(fullPath);

	_sounds.emplace(key, snd);
	return snd;
}
