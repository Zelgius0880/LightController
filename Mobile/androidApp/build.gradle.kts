import com.android.build.gradle.internal.cxx.configure.gradleLocalProperties

plugins {
    id("com.android.application")
    alias(libs.plugins.composeMultiplatform)
    alias(libs.plugins.composeCompiler)
    id("com.google.gms.google-services")
    id("com.google.firebase.crashlytics")
}

android {
    namespace = "com.zelgius.lightcontroller"
    compileSdk = libs.versions.android.compileSdk.get().toInt()

    defaultConfig {
        minSdk = libs.versions.android.minSdk.get().toInt()
        targetSdk = libs.versions.android.targetSdk.get().toInt()
        versionCode = 1
        versionName = "1.0.0"
    }
    packaging {
        resources {
            excludes += "/META-INF/{AL2.0,LGPL2.1}"
        }
    }


    signingConfigs {
        register("release").configure {
            // Read signing configuration from local.properties
            val storeFilePath = gradleLocalProperties(rootDir, providers).getProperty("RELEASE_STORE_FILE")
            if (storeFilePath.isNotEmpty()) {
                storeFile =
                    file(storeFilePath) // Assuming storeFilePath is relative to the app module or an absolute path
            }
            storePassword = gradleLocalProperties(rootDir, providers).getProperty("RELEASE_KEY_PASSWORD")
            keyAlias = gradleLocalProperties(rootDir, providers).getProperty("RELEASE_KEY_ALIAS")
            keyPassword = gradleLocalProperties(rootDir, providers).getProperty("RELEASE_KEY_PASSWORD")
        }
    }

    buildTypes {
        getByName("release") {
            isMinifyEnabled = true
            isDebuggable = false
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro",
            )
            signingConfig = signingConfigs.getByName("release")
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
}

dependencies {
    implementation(projects.composeApp)
    implementation(libs.androidx.activity.compose)
    implementation(libs.androidx.appcompat)
    implementation(libs.androidx.core.ktx)

    implementation(project.dependencies.platform(libs.firebase.bom))
    implementation(libs.firebase.crashlytics)
}

