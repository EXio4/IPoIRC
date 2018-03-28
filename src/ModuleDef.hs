{-# LANGUAGE DataKinds, KindSignatures #-}
module ModuleDef (GenModule(..), Module(..), GModule(..), EdgeModule(..), MiddleModule(..)) where

-- this module defines the low-level interface used by the Architecture to interact/handle modules

import Data.ByteString (ByteString)
import GHC.TypeNats (Nat)
import Data.Text (Text)

data GenModule = GenModule {
    module_name :: Text,
    root_init :: IO (),
    user_init :: IO ()
}

data Side = SInput | SOutput
    
data Buffer (n :: Nat) (side :: Side)
        = EmptyBuffer
        | Data !ByteString 
        
data EdgeModule = EdgeModule {
    etick :: (Buffer 0 SInput) -> IO (Buffer 0 SOutput)
}

data MiddleModule = MiddleModule {
    mtick :: (Buffer 0 SInput, Buffer 1 SInput) -> IO (Buffer 0 SOutput, Buffer 1 SOutput)
}

data GModule = Edge !EdgeModule
             | Middle !MiddleModule
data Module = Module !GenModule !GModule
