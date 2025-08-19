#pragma once

class ResourceBase;
class Texture;
class Sprite;
class Flipbook;
class Tilemap;
class Sound;

class ResourceManager
{
public:
	//DECLARE_SINGLE(ResourceManager);

	~ResourceManager();

public:
	void Init(HWND hwnd, fs::path resourcePath);
	void Clear();

	const fs::path& GetResourcePath() { return _resourcePath; }

	shared_ptr<Texture> GetTexture(const wstring& key) { return _textures[key]; }//key없으면 만들어서 문제..
	shared_ptr<Texture> GetTexture(const wstring& key) const;
	shared_ptr<Texture> LoadTexture(const wstring& key, const wstring& path, uint32 transparent = RGB(255, 0, 255));
	
	shared_ptr<Sprite> GetSprite(const wstring& key)const;
	shared_ptr<Sprite> CreateSprite(const wstring& key, shared_ptr<Texture> texture, int32 x = 0, int32 y = 0, int32 cx = 0, int32 cy = 0);

	shared_ptr<Flipbook> GetFlipbook(const wstring& key)const;
	shared_ptr<Flipbook> CreateFlipbook(const wstring& key);

	shared_ptr<Tilemap> GetTilemap(const wstring& key)const;
	shared_ptr<Tilemap> CreateTilemap(const wstring& key);
	void SaveTilemap(const wstring& key, const wstring& path);
	shared_ptr<Tilemap> LoadTilemap(const wstring& key, const wstring& path);
		
	shared_ptr<Sound> GetSound(const wstring& key)const;
	shared_ptr<Sound> LoadSound(const wstring& key, const wstring& path);

private:
	HWND _hwnd;
	fs::path _resourcePath;

	//unordered_map<wstring, Texture*> _textures;
	unordered_map<wstring, shared_ptr<Texture>> _textures;
	//unordered_map<wstring, Sprite*> _sprites;
	unordered_map<wstring, shared_ptr<Sprite>> _sprites;
	//unordered_map<wstring, Flipbook*> _flipbooks;
	unordered_map<wstring, shared_ptr<Flipbook>> _flipbooks;
	//unordered_map<wstring, Tilemap*> _tilemaps;
	unordered_map<wstring, shared_ptr<Tilemap>> _tilemaps;
	//unordered_map<wstring, Sound*> _sounds;
	unordered_map<wstring, shared_ptr<Sound>> _sounds;
};

