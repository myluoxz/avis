#include "Engine.h"

#ifdef PLATFORM_WIN
#pragma comment(lib, "jpeg_win.lib")
#endif
#include "jpeglib.h"
#include "jerror.h"
#include "lodepng.h"

bool LoadJPEG(string fileN, uint &x, uint &y, byte& channels, byte** data)
{
	//unsigned int texture_id;
	unsigned long data_size;     // length
	unsigned char * rowptr[1];
	struct jpeg_decompress_struct info; //for our jpeg info
	struct jpeg_error_mgr err;          //the error handler

	FILE* file;
#ifdef PLATFORM_WIN
	fopen_s(&file, fileN.c_str(), "rb");  //open the file
#else
	file = fopen(fileN.c_str(), "rb");
#endif
	info.err = jpeg_std_error(&err);
	jpeg_create_decompress(&info);   //fills info structure

									 //if the jpeg file doesn't load
	if (!file) {
		return false;
	}

	jpeg_stdio_src(&info, file);
	jpeg_read_header(&info, TRUE);   // read jpeg file header

	jpeg_start_decompress(&info);    // decompress the file

	x = info.output_width;
	y = info.output_height;
	channels = info.num_components;

	data_size = x * y * 3;

	*data = (unsigned char *)malloc(data_size);
	while (info.output_scanline < y) // loop
	{
		// Enable jpeg_read_scanlines() to fill our jdata array
		rowptr[0] = (unsigned char *)*data + (3 * x * (y - info.output_scanline - 1));

		jpeg_read_scanlines(&info, rowptr, 1);
	}
	//---------------------------------------------------

	jpeg_finish_decompress(&info);   //finish decompressing
	jpeg_destroy_decompress(&info);
	fclose(file);
	return true;
}

//slow!!
void InvertPNG(std::vector<byte>& data, uint x, uint y) {
	for (uint a = 0; a < y/2; a++) {
		for (uint b = 0; b < x; b++) {
			for (uint c = 0; c < 4; c++) {
				byte t = data[(a*x + b) * 4 + c];
				data[(a*x + b) * 4 + c] = data[((y - a - 1)*x + b) * 4 + c];
				data[((y - a - 1)*x + b) * 4 + c] = t;
			}
		}
	}
}

bool LoadPNG(string fileN, uint &x, uint &y, byte& channels, std::vector<byte>& data) {
	channels = 4;
	uint err = lodepng::decode(data, x, y, fileN.c_str());
	if (err) {
		Debug::Error("PNG reader", "Read PNG error: " + string(lodepng_error_text(err)));
		return false;
	}
	InvertPNG(data, x, y);
	return true;
}

bool LoadBMP(string fileN, uint &x, uint &y, byte& channels, byte** data) {

	char header[54]; // Each BMP file begins by a 54-bytes header
	unsigned int dataPos;     // Position in the file where the actual data begins
	unsigned int imageSize;   // = width*height*3
	unsigned short bpi;

	std::ifstream strm(fileN.c_str(), std::ios::in | std::ios::binary);

	if (!strm.is_open()) {
		printf("Image could not be opened\n");
		return false;
	}
	strm.read(header, 54);
	if (strm.bad()) { // If not 54 bytes read : problem
		printf("Not a correct BMP file\n");
		return false;
	}
	if (header[0] != 'B' || header[1] != 'M') {
		printf("Not a correct BMP file\n");
		return false;
	}
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	x = *(int*)&(header[0x12]);
	y = *(int*)&(header[0x16]);
	bpi = *(short*)&(header[0x1c]);
	if (bpi != 24 && bpi != 32)
		return false;
	else channels = (bpi == 24) ? 3 : 4;
	// Some BMP files are misformatted, guess missing information
	if (imageSize == 0)    imageSize = x * y * channels; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54; // The BMP header is done that way
	*data = new unsigned char[imageSize];
	// Read the actual data from the file into the buffer
	strm.read(*(char**)data, imageSize);
#ifdef PLATFORM_ADR
	for (uint a = 0; a < x * y; a++) {
		unsigned char b = (*data)[a];
		(*data)[a] = (*data)[a + 2];
		(*data)[a + 2] = b;
	}
#endif
	return true;
}

byte* Texture::LoadPixels(const string& path, byte& chn, uint& w, uint& h) {
	string sss = path.substr(path.find_last_of('.'), string::npos);
	byte *data;
	std::vector<byte> dataV;
	//std::cout << "opening image at " << path << std::endl;
	//GLenum rgb = GL_RGB, rgba = GL_RGBA;
	if (sss == ".bmp") {
		if (!LoadBMP(path, w, h, chn, &data)) {
			std::cout << "load bmp failed! " << path << std::endl;
			return nullptr;
		}
	}
	else if (sss == ".jpg") {
		if (!LoadJPEG(path, w, h, chn, &data)) {
			std::cout << "load jpg failed! " << path << std::endl;
			return nullptr;
		}
	}
	else if (sss == ".png") {
		if (!LoadPNG(path, w, h, chn, dataV)) {
			std::cout << "load png failed! " << path << std::endl;
			return nullptr;
		}
		data = new byte[w*h*chn];
		memcpy(data, &dataV[0], w*h*chn);
	}
	else {
		std::cout << "Image extension invalid! " << path << std::endl;
		return nullptr;
	}
	return data;
}

byte* Texture::LoadPixels(const byte* data, const uint dataSz, uint& w, uint& h) {
	std::vector<byte> dataV;
	byte chn;
	uint err = lodepng::decode(dataV, w, h, data, dataSz);
	if (!!err) {
		Debug::Error("PNG reader", "Read PNG error: " + string(lodepng_error_text(err)));
		return nullptr;
	}
	InvertPNG(dataV, w, h);
	byte* res = new byte[w * h * 4];
	memcpy(res, &dataV[0], w*h * 4);
	return res;
}

void Texture::ToPNG(std::vector<byte>& data, uint w, uint h, const string& loc) {
	std::vector<byte> res;
	InvertPNG(data, w, h);
	lodepng::encode(res, &data[0], w, h);
	std::ofstream strm(loc, std::ios::binary);
	strm.write((char*)&res[0], res.size());
}

Texture::Texture(const string& path, bool mipmap, TEX_FILTERING filter, byte aniso, TEX_WRAPING warp) :
	Texture(path, mipmap, filter, aniso, (warp == TEX_WRAP_CLAMP) ? GL_CLAMP_TO_EDGE : GL_REPEAT, (warp == TEX_WRAP_CLAMP) ? GL_CLAMP_TO_EDGE : GL_REPEAT) {}

Texture::Texture(const string& path, bool mipmap, TEX_FILTERING filter, byte aniso, GLenum wrapx, GLenum wrapy) : AssetObject(ASSETTYPE_TEXTURE), _mipmap(mipmap), _filter(filter), _aniso(aniso) {
	string sss = path.substr(path.find_last_of('.') + 1, string::npos);
	byte *data;
	std::vector<byte> dataV;
	byte chn;
	//std::cout << "opening image at " << path << std::endl;
	GLenum rgb = GL_RGB, rgba = GL_RGBA;
	if (sss == "bmp") {
		if (!LoadBMP(path, width, height, chn, &data)) {
			Debug::Warning("Texture", "load bmp failed! " + path);
			return;
		}
		rgb = GL_BGR;
		rgba = GL_BGRA;
	}
	else if (sss == "jpg") {
		if (!LoadJPEG(path, width, height, chn, &data)) {
			Debug::Warning("Texture", "load jpg failed! " + path);
			return;
		}
	}
	else if (sss == "png") {
		if (!LoadPNG(path, width, height, chn, dataV)) {
			Debug::Warning("Texture", "load png failed! " + path);
			return;
		}
		data = &dataV[0];
	}
	else {
		Debug::Warning("Texture", "invalid extension! " + sss);
		return;
	}

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	if (chn == 3) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, rgb, GL_UNSIGNED_BYTE, data);
	else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, rgba, GL_UNSIGNED_BYTE, data);
	if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmap && (filter == TEX_FILTER_TRILINEAR)) ? GL_LINEAR_MIPMAP_LINEAR : (filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapx);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapy);
	glBindTexture(GL_TEXTURE_2D, 0);
	if (dataV.size() == 0) delete[](data);
	loaded = true;
}

Texture::Texture(const byte* data, const uint dataSz, TEX_FILTERING filter, TEX_WRAPING wrap) : AssetObject(ASSETTYPE_TEXTURE) {
	std::vector<byte> dataV;
	byte chn;
	uint err = lodepng::decode(dataV, width, height, data, dataSz);
	if (!!err) {
		Debug::Error("PNG reader", "Read PNG error: " + string(lodepng_error_text(err)));
		return;
	}
	InvertPNG(dataV, width, height);

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &dataV[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (wrap == TEX_WRAP_CLAMP) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (wrap == TEX_WRAP_CLAMP) ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);
	loaded = true;
}

Texture::Texture(std::istream& strm, uint offset) : AssetObject(ASSETTYPE_TEXTURE) {
	if (strm.good()) {
		strm.seekg(offset);
		byte chn;
		GLenum rgb = GL_RGB, rgba = GL_RGBA;
		byte* data;
		TEX_TYPE t = _ReadStrm(this, strm, chn, rgb, rgba);
		if (t == TEX_TYPE_UNDEF) {
			Debug::Warning("Texture", "Texture header wrong/missing!");
			return;
		}
		if (t == TEX_TYPE_RENDERTEXTURE) {
			((RenderTexture*)this)->Load(strm);
			return;
		}
		data = new byte[chn*width*height];
		strm.read((char*)data, chn*width*height);
		//strm.close();

		uint mips = 0;
		std::vector<RenderTexture*> rts = std::vector<RenderTexture*>();
		std::vector<Vec2> szs = std::vector<Vec2>();
		if (_mipmap && _blurmips) {
			uint width_1 = width, height_1 = height, width_2, height_2;

			Shader shd = Shader(Camera::d_blurProgram);
			Material mat = Material(&shd);

			rts.push_back(new RenderTexture(width, height, RT_FLAG_NONE, data, (chn == 3) ? GL_RGB : GL_RGBA));
			szs.push_back(Vec2(width, height));

			while (mips < 6 && height_1 > 16) {
				width_2 = width_1 / 2;
				height_2 = height_1 / 2;

				mat.SetFloat("mul", 1 - mips*0.1f);
				mat.SetFloat("mul", 1);
				mat.SetVec2("screenSize", Vec2(width_2, height_2));
				RenderTexture rx = RenderTexture(width_2, height_2, RT_FLAG_HDR);
				mat.SetFloat("isY", 0);
				RenderTexture::Blit(rts[mips], &rx, &mat);
				rts.push_back(new RenderTexture(width_2, height_2, RT_FLAG_HDR));
				mat.SetFloat("isY", 1);
				RenderTexture::Blit(&rx, rts[mips + 1], &mat);

				szs.push_back(Vec2(width_2, height_2));
				width_1 = width_2 + 0;
				height_1 = height_2 + 0;
				mips++;
			}
		}

		glGenTextures(1, &pointer);
		glBindTexture(GL_TEXTURE_2D, pointer);
		if (chn == 3) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, rgb, GL_UNSIGNED_BYTE, data);
		else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, rgba, GL_UNSIGNED_BYTE, data);
		if (_mipmap) {
			if (_blurmips) {
				for (uint a = 1; a <= mips; a++) {
					glBindFramebuffer(GL_READ_FRAMEBUFFER, rts[a]->d_fbo);
					glCopyTexImage2D(GL_TEXTURE_2D, a, GL_RGBA, 0, 0, (GLsizei)szs[a].x, (GLsizei)szs[a].y, 0);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mips);
					delete(rts[a]);
				}

			}
			else glGenerateMipmap(GL_TEXTURE_2D);
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (_mipmap && (_filter == TEX_FILTER_TRILINEAR)) ? GL_LINEAR_MIPMAP_LINEAR : (_filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (_filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, _aniso);
		glBindTexture(GL_TEXTURE_2D, 0);
		delete[](data);
		loaded = true;
	}
}

Texture::Texture(byte* mem) : AssetObject(ASSETTYPE_TEXTURE) {
#ifndef IS_EDITOR
#define RD(tar, sz) memcpy(tar, mem, sz); mem += sz;
	byte b, mips, chn;
	GLenum rgb, rgba;
	RD(&width, 4);
	RD(&height, 4);
	RD(&_filter, 1);
	RD(&_aniso, 1);
	RD(&b, 1);
#define SR(n) (!!((b & (1 << n)) >> n))
	mips = SR(7);
	_repeat = SR(6);
	_blurmips = SR(5);
	chn = 3 + SR(4);
	rgb = SR(3) ? GL_RGB : GL_BGR;
	rgba = SR(3) ? GL_RGBA : GL_BGRA;

	glGenTextures(1, &pointer);
	glBindTexture(GL_TEXTURE_2D, pointer);
	if (chn == 3) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, rgb, GL_UNSIGNED_BYTE, mem);
	else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, rgba, GL_UNSIGNED_BYTE, mem);
	mem += width*height*chn;
	if (_mipmap) {
		if (_blurmips) {
			uint w2, h2;
			for (uint a = 1; a <= mips; a++) {
				RD(&w2, 4);
				RD(&h2, 4);
				if (chn == 3) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w2, h2, 0, rgb, GL_UNSIGNED_BYTE, mem);
				else glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w2, h2, 0, rgba, GL_UNSIGNED_BYTE, mem);
				mem += w2*h2*chn;
			}
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mips);
		}
		else glGenerateMipmap(GL_TEXTURE_2D);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (_mipmap && (_filter == TEX_FILTER_TRILINEAR)) ? GL_LINEAR_MIPMAP_LINEAR : (_filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (_filter == TEX_FILTER_POINT) ? GL_NEAREST : GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, _aniso);
	glBindTexture(GL_TEXTURE_2D, 0);
	loaded = true;
#undef SR
#undef RD
#endif
}

TEX_TYPE Texture::_ReadStrm(Texture* tex, std::istream& strm, byte& chn, GLenum& rgb, GLenum& rgba) {
	std::vector<char> hd(4);
	strm.read((&hd[0]), 3);
	if (hd[0] == 'I' && hd[1] == 'M' && hd[2] == 'R') {
		return TEX_TYPE_RENDERTEXTURE;
	}
	else if (hd[0] != 'I' || hd[1] != 'M' || hd[2] != 'G') {
		Debug::Error("Image Cacher", "Image cache header wrong!");
		return TEX_TYPE_UNDEF;
	}
	byte bb;
	_Strm2Val(strm, chn);
	_Strm2Val(strm, tex->width);
	_Strm2Val(strm, tex->height);
	_Strm2Val(strm, bb);
	if (bb == 1) {
		rgb = GL_BGR;
		rgba = GL_BGRA;
	}
	_Strm2Val(strm, tex->_aniso);
	_Strm2Val(strm, bb);
	tex->_filter = (TEX_FILTERING)bb;
	_Strm2Val(strm, bb);
	tex->_mipmap = !!(bb & 0x80);
	tex->_repeat = !!(bb & 0x01);
	tex->_blurmips = !(bb & 0x10);
	strm.read((&hd[0]), 4);
	if (hd[0] != 'D' || hd[1] != 'A' || hd[2] != 'T' || hd[3] != 'A') {
		Debug::Error("Image Cacher", "Image cache data tag wrong!");
		return TEX_TYPE_NORMAL;
	}
	return TEX_TYPE_NORMAL;
}

bool Texture::DrawPreview(uint x, uint y, uint w, uint h) {
	UI::Texture((float)x, (float)y, (float)w, (float)h, this, DRAWTEX_FIT);
	return true;
}