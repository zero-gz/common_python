// Fill out your copyright notice in the Description page of Project Settings.


#include "MyActor.h"
#include "Templates/SharedPointer.h"
#include "ImageWrapper/Public/IImageWrapper.h"
#include "ImageWrapper/Public/IImageWrapperModule.h"
#include "AssetRegistryModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyFirstActor, Log, All);

// Sets default values
AMyActor::AMyActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UTexture2D> Texture(TEXT("/Game/test_texture"));
	if (Texture.Succeeded())
	{
		texture = Texture.Object;
	}

	UE_LOG(LogMyFirstActor, Log, TEXT("first actor in init function!"));

	//FString test_str(TEXT("abcd"));
	FString test_str = "abedasdf";
	UE_LOG(LogMyFirstActor, Log, TEXT("get a log output:%s"), *test_str);

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("in MyActor init function")));
}

// failed, bulkData is nullptr
void AMyActor::ReadTextureData(UTexture2D* MyTexture2D)
{
	TextureCompressionSettings OldCompressionSettings = MyTexture2D->CompressionSettings; 
	TextureMipGenSettings OldMipGenSettings = MyTexture2D->MipGenSettings;
	bool OldSRGB = MyTexture2D->SRGB;

	MyTexture2D->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	MyTexture2D->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
	MyTexture2D->SRGB = false;
	MyTexture2D->UpdateResource();

	const FColor* FormatedImageData = static_cast<const FColor*>(MyTexture2D->PlatformData->Mips[0].BulkData.LockReadOnly());

	for (int32 X = 0; X < MyTexture2D->GetSizeX(); X++)
	{
		for (int32 Y = 0; Y < MyTexture2D->GetSizeY(); Y++)
		{
			FColor PixelColor = FormatedImageData[Y * MyTexture2D->GetSizeX() + X];
			if (X == 1 && Y == 1)
				UE_LOG(LogMyFirstActor, Log, TEXT("get Color ok!"));
				//UE_LOG(LogMyFirstActor, Log, TEXT("get (1, 1) Color:%s"), *(PixelColor.ToString()));
		}
	}

	MyTexture2D->PlatformData->Mips[0].BulkData.Unlock();

	MyTexture2D->CompressionSettings = OldCompressionSettings;
	MyTexture2D->MipGenSettings = OldMipGenSettings;
	MyTexture2D->SRGB = OldSRGB;
	MyTexture2D->UpdateResource();
}

void AMyActor::WriteTextureData(FString PackageName, FString TextureName)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("in MyActor write_texture function")));
	//创建名为PackageName值的包
//PackageName为FString类型
	FString AssetPath = TEXT("/Game/") + PackageName + TEXT("/");
	AssetPath += TextureName;
	UPackage* Package = CreatePackage(NULL, *AssetPath);
	Package->FullyLoad();

	int32 TextureWidth = 1024;
	int32 TextureHeight = 768;

	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *TextureName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
	NewTexture->AddToRoot();
	NewTexture->PlatformData = new FTexturePlatformData();
	NewTexture->PlatformData->SizeX = TextureWidth;
	NewTexture->PlatformData->SizeY = TextureHeight;
	NewTexture->PlatformData->SetNumSlices(1);
	//设置像素格式
	NewTexture->PlatformData->PixelFormat = EPixelFormat::PF_B8G8R8A8;

	//创建一个uint8的数组并取得指针
//这里需要考虑之前设置的像素格式
	uint8* Pixels = new uint8[TextureWidth * TextureHeight * 4];
	for (int32 y = 0; y < TextureHeight; y++)
	{
		for (int32 x = 0; x < TextureWidth; x++)
		{
			int32 curPixelIndex = ((y * TextureWidth) + x);
			//这里可以设置4个通道的值
			//这里需要考虑像素格式，之前设置了PF_B8G8R8A8，那么这里通道的顺序就是BGRA
			Pixels[4 * curPixelIndex] = 0;
			Pixels[4 * curPixelIndex + 1] = 0;
			Pixels[4 * curPixelIndex + 2] = 255;
			Pixels[4 * curPixelIndex + 3] = 255;
		}
	}
	//创建第一个MipMap
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	NewTexture->PlatformData->Mips.Add(Mip);
	Mip->SizeX = TextureWidth;
	Mip->SizeY = TextureHeight;

	//锁定Texture让它可以被修改
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	uint8* TextureData = (uint8*)Mip->BulkData.Realloc(TextureWidth * TextureHeight * 4);
	FMemory::Memcpy(TextureData, Pixels, sizeof(uint8) * TextureHeight * TextureWidth * 4);
	Mip->BulkData.Unlock();

	//通过以上步骤，我们完成数据的临时写入
	//执行完以下这两个步骤，编辑器中的asset会显示可以保存的状态（如果是指定Asset来获取UTexture2D的指针的情况下）
	NewTexture->MipGenSettings = TMGS_NoMipmaps;
	NewTexture->SRGB = false;

	NewTexture->Source.Init(TextureWidth, TextureHeight, 1, 1, ETextureSourceFormat::TSF_BGRA8, Pixels);
	NewTexture->UpdateResource();

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewTexture);
	//通过asset路径获取包中文件名
	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	//进行保存
	bool bSaved = UPackage::SavePackage(Package, NewTexture, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_NoError);

	delete[] Pixels;
}

UTexture2D* AMyActor::LoadTexByPath(FString ImagePath)
{
	UTexture2D* Texture = nullptr;
	bool IsValid = false;
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*ImagePath))
	{
		return nullptr;
	}
	TArray<uint8> CompressedData;
	if (!FFileHelper::LoadFileToArray(CompressedData, *ImagePath))
	{
		return nullptr;
	}

	FString Ex = FPaths::GetExtension(ImagePath, false);
	EImageFormat ImageFormat = EImageFormat::Invalid;
	if (Ex.Equals(TEXT("png"), ESearchCase::IgnoreCase))
	{
		ImageFormat = EImageFormat::PNG;
	}

	if (ImageFormat == EImageFormat::Invalid)
		return nullptr;

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);

	if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(CompressedData.GetData(), CompressedData.Num()))
	{
		//TArray<uint8>* UncompressedRGBA = nullptr;
		TArray<uint8> UncompressedRGBA;
		if (ImageWrapper->GetRaw(ERGBFormat::RGBA, 8, UncompressedRGBA))
		{
			UE_LOG(LogMyFirstActor, Log, TEXT("get raw data is ok!"));
			Texture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_R8G8B8A8);
			if (Texture != nullptr)
			{
				IsValid = true;
				int32 OutWidth = ImageWrapper->GetWidth();
				int32 OutHeight = ImageWrapper->GetHeight();
				void* TextureData = Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(TextureData, UncompressedRGBA.GetData(), UncompressedRGBA.Num());
				Texture->PlatformData->Mips[0].BulkData.Unlock();
				Texture->UpdateResource();
			}
		}
	}
	return Texture;
}

// Called when the game starts or when spawned
void AMyActor::BeginPlay()
{
	Super::BeginPlay();
	mt = LoadObject<UMaterial>(nullptr, TEXT("/Game/StarterContent/Materials/M_Basic_Floor"));
}

// Called every frame
void AMyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

