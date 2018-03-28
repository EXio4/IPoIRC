module Architecture where

import ModuleDef
import qualified Data.Map as M
import qualified Data.Vector as V
import Control.Monad
import Control.Concurrent.STM.TVar

data System = System !(M.Map Int Module) !(TVar (V.Vector (Int,Int)))

create :: [(Int, Module)] -> IO System
create xs = do
    v <- newTVarIO V.empty
    return $ System (M.fromList xs) v

perModule :: ((Int, Module) -> IO ()) -> System -> IO ()
perModule cb (System m v) = do
    forM_ (M.toList m) $ \m -> 
        cb m

rootInit :: System -> IO ()
rootInit = perModule (\(_,Module g _) -> root_init g)

normalInit :: System -> IO ()
normalInit = perModule (\(_,Module g _) -> user_init g)
