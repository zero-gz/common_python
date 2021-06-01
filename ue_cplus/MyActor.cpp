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
	//������ΪPackageNameֵ�İ�
//PackageNameΪFString����
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
	//�������ظ�ʽ
	NewTexture->PlatformData->PixelFormat = EPixelFormat::PF_B8G8R8A8;

	//����һ��uint8�����鲢ȡ��ָ��
//������Ҫ����֮ǰ���õ����ظ�ʽ
	uint8* Pixels = new uint8[TextureWidth * TextureHeight * 4];
	for (int32 y = 0; y < TextureHeight; y++)
	{
		for (int32 x = 0; x < TextureWidth; x++)
		{
			int32 curPixelIndex = ((y * TextureWidth) + x);
			//�����������4��ͨ����ֵ
			//������Ҫ�������ظ�ʽ��֮ǰ������PF_B8G8R8A8����ô����ͨ����˳�����BGRA
			Pixels[4 * curPixelIndex] = 0;
			Pixels[4 * curPixelIndex + 1] = 0;
			Pixels[4 * curPixelIndex + 2] = 255;
			Pixels[4 * curPixelIndex + 3] = 255;
		}
	}
	//������һ��MipMap
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	NewTexture->PlatformData->Mips.Add(Mip);
	Mip->SizeX = TextureWidth;
	Mip->SizeY = TextureHeight;

	//����Texture�������Ա��޸�
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	uint8* TextureData = (uint8*)Mip->BulkData.Realloc(TextureWidth * TextureHeight * 4);
	FMemory::Memcpy(TextureData, Pixels, sizeof(uint8) * TextureHeight * TextureWidth * 4);
	Mip->BulkData.Unlock();

	//ͨ�����ϲ��裬����������ݵ���ʱд��
	//ִ�����������������裬�༭���е�asset����ʾ���Ա����״̬�������ָ��Asset����ȡUTexture2D��ָ�������£�
	NewTexture->MipGenSettings = TMGS_NoMipmaps;
	NewTexture->SRGB = false;

	NewTexture->Source.Init(TextureWidth, TextureHeight, 1, 1, ETextureSourceFormat::TSF_BGRA8, Pixels);
	NewTexture->UpdateResource();

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewTexture);
	//ͨ��asset·����ȡ�����ļ���
	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
	//���б���
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

