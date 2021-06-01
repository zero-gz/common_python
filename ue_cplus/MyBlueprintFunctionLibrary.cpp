// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBlueprintFunctionLibrary.h"

#include "ImageWrapper/Public/IImageWrapper.h"
#include "ImageWrapper/Public/IImageWrapperModule.h"
#include "AssetRegistryModule.h"
#include "AssetTools/Public/AssetToolsModule.h"
#include "ImageUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyLib, Log, All);

void UMyBlueprintFunctionLibrary::ReadTextureDataByFile(FString ImagePath, TArray<uint8>& colors, int32& TextureWidth, int32& TextureHeight)
{
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*ImagePath))
	{
		return;
	}
	TArray<uint8> CompressedData;
	if (!FFileHelper::LoadFileToArray(CompressedData, *ImagePath))
	{
		return;
	}

	FString Ex = FPaths::GetExtension(ImagePath, false);
	EImageFormat ImageFormat = EImageFormat::Invalid;
	if (Ex.Equals(TEXT("png"), ESearchCase::IgnoreCase))
	{
		ImageFormat = EImageFormat::PNG;
	}
	else if (Ex.Equals(TEXT("bmp"), ESearchCase::IgnoreCase))
	{
		ImageFormat = EImageFormat::BMP;
	}

	if (ImageFormat == EImageFormat::Invalid)
		return;

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);

	if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(CompressedData.GetData(), CompressedData.Num()))
	{
		//TArray<uint8>* UncompressedRGBA = nullptr;
		ImageWrapper->GetRaw(ERGBFormat::RGBA, 8, colors);
		TextureWidth = ImageWrapper->GetWidth();
		TextureHeight = ImageWrapper->GetHeight();
	}
}

void UMyBlueprintFunctionLibrary::ReadTextureDataByTexture2D(UTexture2D* TextureToRead, TArray<uint8>& colors, int32& TextureWidth, int32&TextureHeight, int MipLevel)
{
	if (MipLevel >= TextureToRead->PlatformData->Mips.Num())
	{
		UE_LOG(LogMyLib, Error, TEXT("texture2D not have enough Miplevel"));
		return;
	}

	TextureCompressionSettings OldCompressionSettings = TextureToRead->CompressionSettings;
	TextureMipGenSettings OldMipGenSettings = TextureToRead->MipGenSettings;
	bool OldSRGB = TextureToRead->SRGB;

	TextureToRead->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	TextureToRead->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
	TextureToRead->SRGB = false;
	TextureToRead->UpdateResource();

	FTexture2DMipMap& mipmap = TextureToRead->PlatformData->Mips[MipLevel];
	void* Data = mipmap.BulkData.Lock(LOCK_READ_WRITE);
	uint32* PixelPointer = (uint32*)Data;
	if (PixelPointer == nullptr)
	{
		mipmap.BulkData.Unlock();
		return;
	}

	TextureWidth = TextureToRead->PlatformData->SizeX;
	TextureHeight = TextureToRead->PlatformData->SizeY;
	colors.AddUninitialized(TextureWidth * TextureHeight * 4);

	for (int32 x = 0; x < TextureWidth * TextureHeight; x++)
	{
		uint32 EncodedPixel = *PixelPointer;
		//R
		colors[4 * x] = (EncodedPixel & 0x00FF0000) >> 16; 

		//G
		colors[4 * x + 1] = (EncodedPixel & 0x0000FF00) >> 8;

		//B
		colors[4 * x + 2] = (EncodedPixel & 0x000000FF);

		//A
		colors[4 * x + 3] = (EncodedPixel & 0xFF000000) >> 24;

		PixelPointer++;
	}
	mipmap.BulkData.Unlock();

	TextureToRead->CompressionSettings = OldCompressionSettings;
	TextureToRead->MipGenSettings = OldMipGenSettings;
	TextureToRead->SRGB = OldSRGB;
	TextureToRead->UpdateResource();
}

UTexture2D* UMyBlueprintFunctionLibrary::WriteTexture2DData(UTexture2D* TextureToWrite, TArray<uint8> colors, int32 TextureWidth, int32 TextureHeight, int MipLevel)
{
	TextureCompressionSettings OldCompressionSettings = TextureToWrite->CompressionSettings;
	TextureMipGenSettings OldMipGenSettings = TextureToWrite->MipGenSettings;
	bool OldSRGB = TextureToWrite->SRGB;

	TextureToWrite->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	TextureToWrite->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
	TextureToWrite->SRGB = false;
	TextureToWrite->UpdateResource();

	FTexture2DMipMap& mipmap = TextureToWrite->PlatformData->Mips[MipLevel];
	
	
	//void* Data = mipmap.BulkData.Lock(LOCK_READ_WRITE);
	//mipmap.BulkData.Realloc(TextureWidth * TextureHeight * 4);
	//uint8* TextureData = (uint8*)mipmap.BulkData.Realloc(TextureWidth * TextureHeight * 4);

	//if (TextureData == nullptr)
	//{
	//	mipmap.BulkData.Unlock();
	//	return TextureToWrite;
	//}

	//for (int32 x = 0; x < TextureWidth * TextureHeight*4; x+=4)
	//{
	//	TextureData[x] = colors[x + 2];
	//	TextureData[x + 1] = colors[x + 1];
	//	TextureData[x + 2] = colors[x];
	//	TextureData[x + 3] = colors[x + 3];
	//}
	//mipmap.BulkData.Unlock();

	TArray<uint8> TextureData;
	TextureData.AddUninitialized(TextureWidth * TextureHeight * 4);
	for (int32 x = 0; x < TextureWidth * TextureHeight*4; x+=4)
	{
		TextureData[x] = colors[x + 2];
		TextureData[x + 1] = colors[x + 1];
		TextureData[x + 2] = colors[x];
		TextureData[x + 3] = colors[x + 3];
	}

	TextureToWrite->Source.Init(TextureWidth, TextureHeight, 1, 1, ETextureSourceFormat::TSF_BGRA8, TextureData.GetData());
	TextureToWrite->UpdateResource();

	TextureToWrite->CompressionSettings = OldCompressionSettings;
	TextureToWrite->MipGenSettings = OldMipGenSettings;
	TextureToWrite->SRGB = OldSRGB;
	TextureToWrite->UpdateResource();

	return TextureToWrite;
}

void UMyBlueprintFunctionLibrary::SaveTextureAssets(FString PackageName, FString TextureName, TArray<uint8> colors, int32 TextureWidth, int32 TextureHeight)
{
	TArray<uint8> real_colors;
	real_colors.AddUninitialized(colors.Num());
	for (int i = 0; i < colors.Num(); i+=4)
	{
		real_colors[i] = colors[i+2];
		real_colors[i+1] = colors[i+1];
		real_colors[i+2] = colors[i];
		real_colors[i+3] = colors[i+3];
	}

	FString AssetPath = TEXT("/Game/") + PackageName + TEXT("/");
	AssetPath += TextureName;
	UPackage* Package = CreatePackage(*AssetPath);
	Package->FullyLoad();

	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *TextureName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
	NewTexture->AddToRoot();
	NewTexture->PlatformData = new FTexturePlatformData();
	NewTexture->PlatformData->SizeX = TextureWidth;
	NewTexture->PlatformData->SizeY = TextureHeight;
	NewTexture->PlatformData->SetNumSlices(1);
	//设置像素格式
	NewTexture->PlatformData->PixelFormat = EPixelFormat::PF_B8G8R8A8;

	//创建第一个MipMap
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	NewTexture->PlatformData->Mips.Add(Mip);
	Mip->SizeX = TextureWidth;
	Mip->SizeY = TextureHeight;

	//锁定Texture让它可以被修改
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	uint8* TextureData = (uint8*)Mip->BulkData.Realloc(TextureWidth * TextureHeight * 4);
	FMemory::Memcpy(TextureData, real_colors.GetData(), sizeof(uint8) * TextureHeight * TextureWidth * 4);
	Mip->BulkData.Unlock();

	//通过以上步骤，我们完成数据的临时写入
	//执行完以下这两个步骤，编辑器中的asset会显示可以保存的状态（如果是指定Asset来获取UTexture2D的指针的情况下）
	NewTexture->MipGenSettings = TMGS_NoMipmaps;
	NewTexture->SRGB = false;

	NewTexture->Source.Init(TextureWidth, TextureHeight, 1, 1, ETextureSourceFormat::TSF_BGRA8, real_colors.GetData());
	NewTexture->UpdateResource();

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewTexture);
	//通过asset路径获取包中文件名
	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	//进行保存
	bool bSaved = UPackage::SavePackage(Package, NewTexture, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_NoError);
}

bool UMyBlueprintFunctionLibrary::SavePNGImage(FString ImagePath, TArray<uint8> colors, int32 TextureWidth, int32 TextureHeight)
{
	// has UTexture pointer, can use asset tool
	//FAssetToolsModule& AssetToolModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	//IAssetTools& AssetTool = AssetToolModule.Get();

	//TArray<UObject*>Textures;
	//Textures.Add(UTexture2D);
	//AssetTool.ExportAssetsWithDialog(Textures, true);

	TArray<FColor> FColors;
	int32 FColorLen = colors.Num() / 4;
	FColors.AddUninitialized(FColorLen);
	for (int i = 0; i < FColorLen; i++)
	{
		FColors[i] = FColor(colors[4 * i], colors[4 * i + 1], colors[4 * i + 2], colors[4 * i + 3]);
	}

	FIntPoint destSize(TextureWidth, TextureHeight);
	TArray<uint8> CompressedPNG;
	FImageUtils::CompressImageArray(destSize.X, destSize.Y, FColors, CompressedPNG);
	bool imageSavedOk = FFileHelper::SaveArrayToFile(CompressedPNG, *ImagePath);
	return imageSavedOk;
}
