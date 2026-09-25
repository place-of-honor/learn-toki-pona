plugins {
    id("com.android.application")
}

val requestedVersionCode = System.getenv("PLAY_VERSION_CODE")
val appVersionCode = when {
    requestedVersionCode == null -> 3
    else -> requestedVersionCode.toIntOrNull()
        ?.takeIf { it in 1..2_100_000_000 }
        ?: error("PLAY_VERSION_CODE must be between 1 and 2100000000")
}
val appVersionName = System.getenv("PLAY_VERSION_NAME") ?: "1.0.2"

val allowedNativeAbis = setOf("armeabi-v7a", "arm64-v8a")
val requestedNativeAbis = (System.getenv("TOKI_PONA_ABIS") ?: "armeabi-v7a,arm64-v8a")
    .split(",")
    .map { it.trim() }
    .filter { it.isNotEmpty() }
require(requestedNativeAbis.isNotEmpty()) { "TOKI_PONA_ABIS must name at least one ABI" }
require(requestedNativeAbis.all { it in allowedNativeAbis }) {
    "TOKI_PONA_ABIS may contain only armeabi-v7a and arm64-v8a"
}

val uploadKeystorePath = System.getenv("ANDROID_UPLOAD_KEYSTORE_PATH")
val uploadKeystorePassword = System.getenv("ANDROID_UPLOAD_KEYSTORE_PASSWORD")
val uploadKeyAlias = System.getenv("ANDROID_UPLOAD_KEY_ALIAS")
val uploadKeyPassword = System.getenv("ANDROID_UPLOAD_KEY_PASSWORD")
val uploadSigningConfigured = listOf(
    uploadKeystorePath,
    uploadKeystorePassword,
    uploadKeyAlias,
    uploadKeyPassword,
).all { !it.isNullOrBlank() }

android {
    namespace = "org.tokipona.drills"
    compileSdk = 36
    ndkVersion = "30.0.16248370"

    defaultConfig {
        applicationId = "org.tokipona.drills"
        minSdk = 23
        targetSdk = 36
        versionCode = appVersionCode
        versionName = appVersionName

        ndk {
            abiFilters += requestedNativeAbis
        }
    }

    sourceSets {
        getByName("main") {
            manifest.srcFile("src/gradle/AndroidManifest.xml")
            java.setSrcDirs(emptyList<String>())
            assets.setSrcDirs(emptyList<String>())
        }
    }

    externalNativeBuild {
        cmake {
            path = file("native/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    signingConfigs {
        if (uploadSigningConfigured) {
            create("playUpload") {
                storeFile = file(uploadKeystorePath!!)
                storePassword = uploadKeystorePassword
                keyAlias = uploadKeyAlias
                keyPassword = uploadKeyPassword
            }
        }
    }

    buildTypes {
        getByName("release") {
            isMinifyEnabled = false
            signingConfigs.findByName("playUpload")?.let {
                signingConfig = it
            }
        }
    }

    packaging {
        jniLibs {
            useLegacyPackaging = false
        }
    }
}
