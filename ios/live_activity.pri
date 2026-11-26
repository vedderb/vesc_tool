# Mark Swift inputs as compilable sources for qmake's Xcode generator.
RIDE_SWIFT_SOURCES = $$PWD/LiveActivity/LiveActivityBridge.swift $$PWD/LiveActivity/RideActivityAttributes.swift
ride_swift.input = RIDE_SWIFT_SOURCES
ride_swift.output = ${QMAKE_FILE_BASE}.o
ride_swift.commands = true
QMAKE_EXTRA_COMPILERS += ride_swift

ride_swift_version.name = SWIFT_VERSION
ride_swift_version.value = 5.0
ride_modules.name = CLANG_ENABLE_MODULES
ride_modules.value = YES
ride_swift_runtime.name = ALWAYS_EMBED_SWIFT_STANDARD_LIBRARIES
ride_swift_runtime.value = YES
QMAKE_TARGET_BUNDLE_PREFIX = com.vesc-project
QMAKE_BUNDLE = vesc-tool
ride_version.name = MARKETING_VERSION
ride_version.value = $$VT_VERSION
ride_build.name = CURRENT_PROJECT_VERSION
ride_build.value = $$VT_VERSION
QMAKE_MAC_XCODE_SETTINGS += ride_swift_version ride_modules ride_swift_runtime ride_version ride_build

# Build and embed the native extension before Xcode signs the containing app.
QMAKE_POST_LINK += $$escape_expand(\\n) \
    xcodebuild -quiet -project $$shell_quote($$PWD/LiveActivity/LiveActivity.xcodeproj) \
    -target VESCRideActivity -configuration \"$CONFIGURATION\" -sdk \"$PLATFORM_NAME\" \
    \"ARCHS=$ARCHS\" \"VESC_APP_BUNDLE_ID=$PRODUCT_BUNDLE_IDENTIFIER\" \
    \"MARKETING_VERSION=$MARKETING_VERSION\" \"CURRENT_PROJECT_VERSION=$CURRENT_PROJECT_VERSION\" \
    \"DEVELOPMENT_TEAM=$DEVELOPMENT_TEAM\" \"CODE_SIGN_IDENTITY=$CODE_SIGN_IDENTITY\" \
    \"CODE_SIGNING_ALLOWED=$CODE_SIGNING_ALLOWED\" \
    \"CONFIGURATION_BUILD_DIR=$BUILT_PRODUCTS_DIR/RideActivity\" \"OBJROOT=$OBJROOT/RideActivity\" \
    build && \
    ditto \"$BUILT_PRODUCTS_DIR/RideActivity/VESCRideActivity.appex\" \"$TARGET_BUILD_DIR/$WRAPPER_NAME/PlugIns/VESCRideActivity.appex\"
